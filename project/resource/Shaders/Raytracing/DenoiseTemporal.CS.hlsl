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
    float2 temporalPad;
};

Texture2D<float4> gGBufferNormal : register(t0);
Texture2D<float> gGBufferDepth : register(t1);
Texture2D<float4> gGBufferNormalPrev : register(t2);
Texture2D<float> gGBufferDepthPrev : register(t3);
Texture2D<float> gDenoiseRaw : register(t4);
Texture2D<float2> gDenoiseHistoryPrev : register(t5);

// 次フレームへ渡す履歴と、空間フィルタへ渡す信号でチャンネル数が違うので出力を分ける
RWTexture2D<float2> gDenoiseHistoryOut : register(u0); // x=信号, y=蓄積フレーム数
RWTexture2D<float> gDenoiseFilterOut : register(u1);

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
        gDenoiseHistoryOut[pixel] = float2(1.0f, 0.0f);
        gDenoiseFilterOut[pixel] = 1.0f;
        return;
    }

    float raw = gDenoiseRaw.Load(int3(pixel, 0)).r;
    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);

    float2 history = float2(0.0f, 0.0f);
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

                history += bilinear[i] * gDenoiseHistoryPrev.Load(int3(tap, 0)).xy;
                historyWeight += bilinear[i];
            }
        }
    }

    float filtered = raw;
    float historyLength = 1.0f;

    if (historyWeight > 0.0f)
    {
        history /= historyWeight;
        historyLength = min(history.y + 1.0f, maxHistoryLength);
        // 浅いうちは 1/n の単純平均、上限に達すると指数移動平均になる
        filtered = lerp(history.x, raw, 1.0f / historyLength);
    }

    gDenoiseHistoryOut[pixel] = float2(filtered, historyLength);
    gDenoiseFilterOut[pixel] = filtered;
}
