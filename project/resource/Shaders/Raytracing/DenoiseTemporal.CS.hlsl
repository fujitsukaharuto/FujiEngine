#include "../Common/GBufferUtil.hlsli"

// デノイザの時間方向。前フレームの ViewProjection で再投影して蓄積を引き当て、生の信号と混ぜる。
// AOも影の遮蔽率も視点に依らない量なので、同じ点を写していた画素さえ分かればそのまま使える

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b0)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// Denoiser.h の DenoiseTemporalParam と並びが一致していること
cbuffer DenoiseTemporalParam : register(b1)
{
    float4x4 prevViewProj;
    float4x4 prevInvViewProj;
    uint2 screenSize;
    uint hasHistory;         // 0なら履歴を見ない(初回フレーム/AOを入れ直した直後)
    float maxHistoryLength;  // 蓄積の上限フレーム数。大きいほど滑らかで動きへの追従は遅い
    float normalThreshold;   // 履歴を認める法線の一致度(cos)
    float positionThreshold; // 履歴を認めるワールド座標のずれ。カメラからの距離に対する割合
    uint historyClamp;       // 0以外なら、履歴を近傍の生の信号の範囲へ収める
    float temporalPad;
};

Texture2D<float4> gGBufferNormal : register(t0);
Texture2D<float> gGBufferDepth : register(t1);
Texture2D<float4> gGBufferNormalPrev : register(t2);
Texture2D<float> gGBufferDepthPrev : register(t3);
// DENOISE_RGB を付けると3チャンネルの信号を1回でならす (DenoiseTemporalRGB.CS.hlsl)
#ifdef DENOISE_RGB
typedef float3 DenoiseSignal;
Texture2D<float4> gDenoiseRaw : register(t4);
Texture2D<float4> gDenoiseHistoryPrev : register(t5);
RWTexture2D<float4> gDenoiseHistoryOut : register(u0); // rgb=信号, a=蓄積フレーム数
RWTexture2D<float4> gDenoiseFilterOut : register(u1);

DenoiseSignal LoadRaw(int2 pixel)
{
    return gDenoiseRaw.Load(int3(pixel, 0)).rgb;
}

void LoadHistory(int2 pixel, out DenoiseSignal signal, out float count)
{
    float4 history = gDenoiseHistoryPrev.Load(int3(pixel, 0));
    signal = history.rgb;
    count = history.a;
}

void StoreResult(uint2 pixel, DenoiseSignal signal, float count)
{
    gDenoiseHistoryOut[pixel] = float4(signal, count);
    gDenoiseFilterOut[pixel] = float4(signal, 0.0f);
}

float MaxComponent(DenoiseSignal value)
{
    return max(value.r, max(value.g, value.b));
}
#else
typedef float DenoiseSignal;
Texture2D<float> gDenoiseRaw : register(t4);
Texture2D<float2> gDenoiseHistoryPrev : register(t5);

// 次フレームへ渡す履歴と、空間フィルタへ渡す信号でチャンネル数が違うので出力を分ける
RWTexture2D<float2> gDenoiseHistoryOut : register(u0); // x=信号, y=蓄積フレーム数
RWTexture2D<float> gDenoiseFilterOut : register(u1);

DenoiseSignal LoadRaw(int2 pixel)
{
    return gDenoiseRaw.Load(int3(pixel, 0)).r;
}

void LoadHistory(int2 pixel, out DenoiseSignal signal, out float count)
{
    float2 history = gDenoiseHistoryPrev.Load(int3(pixel, 0)).xy;
    signal = history.x;
    count = history.y;
}

void StoreResult(uint2 pixel, DenoiseSignal signal, float count)
{
    gDenoiseHistoryOut[pixel] = float2(signal, count);
    gDenoiseFilterOut[pixel] = signal;
}

float MaxComponent(DenoiseSignal value)
{
    return value;
}
#endif

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= screenSize.x || pixel.y >= screenSize.y)
    {
        return;
    }

    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        // 面が無い画素(空)。遮蔽しようがないので開けている扱いで、履歴も持たない
        StoreResult(pixel, (DenoiseSignal) 1.0f, 0.0f);
        return;
    }

    DenoiseSignal raw = LoadRaw(int2(pixel));
    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);

    DenoiseSignal historySignal = (DenoiseSignal) 0.0f;
    float historyCount = 0.0f;
    float historyWeight = 0.0f;

    if (hasHistory != 0)
    {
        float4 prevClip = mul(float4(worldPos, 1.0f), prevViewProj);
        if (prevClip.w > 0.0f)
        {
            float3 prevNdc = prevClip.xyz / prevClip.w;
            float2 prevUv = float2(prevNdc.x * 0.5f + 0.5f, 0.5f - prevNdc.y * 0.5f);

            // 画素の中心が0.5なので、2x2の左上を取るには0.5を引いてから床を取る
            float2 prevPixelF = prevUv * float2(screenSize) - 0.5f;
            int2 basePixel = int2(floor(prevPixelF));
            float2 subPixel = prevPixelF - float2(basePixel);

            // 双一次補間の重み。4点のうち一部だけが別の面ということが縁で起きるので1点ずつ確かめる
            float bilinear[4] =
            {
                (1.0f - subPixel.x) * (1.0f - subPixel.y),
                subPixel.x * (1.0f - subPixel.y),
                (1.0f - subPixel.x) * subPixel.y,
                subPixel.x * subPixel.y
            };
            int2 offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };

            // カメラから遠いほど1画素の覆う範囲が広いので、許すずれも比例させる
            float positionTolerance = positionThreshold * length(worldPos - cameraPos);

            for (uint i = 0; i < 4; i++)
            {
                int2 tap = basePixel + offset[i];
                if (tap.x < 0 || tap.y < 0 || tap.x >= int(screenSize.x) || tap.y >= int(screenSize.y))
                {
                    continue;
                }

                float3 prevNormal;
                float prevDepth;
                if (!LoadSurface(gGBufferNormalPrev, gGBufferDepthPrev, tap, prevNormal, prevDepth))
                {
                    continue;
                }
                if (dot(prevNormal, normal) < normalThreshold)
                {
                    continue;
                }

                float3 prevWorldPos = ReconstructWorldPos(uint2(tap), screenSize, prevDepth, prevInvViewProj);
                if (distance(prevWorldPos, worldPos) > positionTolerance)
                {
                    continue;
                }

                DenoiseSignal tapSignal;
                float tapCount;
                LoadHistory(tap, tapSignal, tapCount);
                historySignal += bilinear[i] * tapSignal;
                historyCount += bilinear[i] * tapCount;
                historyWeight += bilinear[i];
            }
        }
    }

    DenoiseSignal filtered = raw;
    float historyLength = 1.0f;

    if (historyWeight > 0.0f)
    {
        historySignal /= historyWeight;
        historyCount /= historyWeight;

        // 再投影はカメラの動きしか追えず、遮蔽物が動くと古い影が残る。
        // 履歴を近傍の生の信号の範囲へ収めて追従させる
        if (historyClamp != 0)
        {
            DenoiseSignal lo = raw;
            DenoiseSignal hi = raw;
            for (int y = -1; y <= 1; y++)
            {
                for (int x = -1; x <= 1; x++)
                {
                    int2 tap = clamp(int2(pixel) + int2(x, y), int2(0, 0), int2(screenSize) - 1);
                    DenoiseSignal v = LoadRaw(tap);
                    lo = min(lo, v);
                    hi = max(hi, v);
                }
            }

            DenoiseSignal clamped = clamp(historySignal, lo, hi);
            // 大きく切り詰めた＝そこの値が変わった。蓄積も浅い扱いへ戻して、以後の追従も速める
            historyCount *= 1.0f - saturate(MaxComponent(abs(clamped - historySignal)) * 4.0f);
            historySignal = clamped;
        }

        historyLength = min(historyCount + 1.0f, maxHistoryLength);
        // 浅いうちは 1/n の単純平均、上限に達すると指数移動平均になる
        filtered = lerp(historySignal, raw, 1.0f / historyLength);
    }

    StoreResult(pixel, filtered, historyLength);
}
