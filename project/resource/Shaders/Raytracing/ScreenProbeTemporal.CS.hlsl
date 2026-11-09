#include "../Common/ScreenProbeCommon.hlsli"

// 画素ごとの間接光を前のフレームの結果と混ぜる (DenoiseTemporal.CS.hlsl と同じ判定)

Texture2D<float4> gScreenGIRaw : register(t8, space7);
Texture2D<float4> gScreenGIHistoryPrev : register(t9, space7);
Texture2D<float> gScreenGICountPrev : register(t10, space7);
RWTexture2D<float4> gScreenGIOut : register(u0);    // rgb=間接光, a=使えるか
RWTexture2D<float> gScreenGICountOut : register(u1); // 蓄積したフレーム数

static const float kNormalThreshold = 0.9f;
// 履歴を認めるワールド座標のずれ。カメラからの距離に対する割合
static const float kPositionThreshold = 0.02f;

float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (any(pixel >= screenSize))
    {
        return;
    }

    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gScreenGIOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        gScreenGICountOut[pixel] = 0.0f;
        return;
    }

    float4 raw = gScreenGIRaw.Load(int3(pixel, 0));
    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);

    float4 history = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float historyCount = 0.0f;
    float historyWeight = 0.0f;
    if (hasHistory != 0)
    {
        float4 prevClip = mul(float4(worldPos, 1.0f), prevViewProj);
        if (prevClip.w > 0.0f)
        {
            float3 prevNdc = prevClip.xyz / prevClip.w;
            float2 prevUv = float2(prevNdc.x * 0.5f + 0.5f, 0.5f - prevNdc.y * 0.5f);
            float2 prevPixelF = prevUv * float2(screenSize) - 0.5f;
            int2 basePixel = int2(floor(prevPixelF));
            float2 subPixel = prevPixelF - float2(basePixel);
            float bilinear[4] =
            {
                (1.0f - subPixel.x) * (1.0f - subPixel.y),
                subPixel.x * (1.0f - subPixel.y),
                (1.0f - subPixel.x) * subPixel.y,
                subPixel.x * subPixel.y
            };
            int2 offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
            float positionTolerance = kPositionThreshold * length(worldPos - cameraPos);

            for (uint i = 0; i < 4; i++)
            {
                int2 tap = basePixel + offset[i];
                if (any(tap < 0) || any(tap >= int2(screenSize)))
                {
                    continue;
                }
                float3 prevNormal;
                float prevDepth;
                if (!LoadSurface(gGBufferNormalPrev, gGBufferDepthPrev, tap, prevNormal, prevDepth) ||
                    dot(prevNormal, normal) < kNormalThreshold)
                {
                    continue;
                }
                float3 prevWorldPos = ReconstructWorldPos(uint2(tap), screenSize, prevDepth, prevInvViewProj);
                if (distance(prevWorldPos, worldPos) > positionTolerance)
                {
                    continue;
                }
                history += bilinear[i] * gScreenGIHistoryPrev.Load(int3(tap, 0));
                historyCount += bilinear[i] * gScreenGICountPrev.Load(int3(tap, 0));
                historyWeight += bilinear[i];
            }
        }
    }

    float4 result = raw;
    float count = 1.0f;
    if (historyWeight > 1e-3f)
    {
        history /= historyWeight;
        historyCount /= historyWeight;
        // 光が大きく変わった所は蓄積を浅くする
        float historyLuminance = Luminance(history.rgb);
        float rawLuminance = Luminance(raw.rgb);
        float ratio = (max(historyLuminance, rawLuminance) + 1e-3f) / (min(historyLuminance, rawLuminance) + 1e-3f);
        if (ratio > 4.0f)
        {
            historyCount = min(historyCount, 2.0f);
        }
        count = min(historyCount + 1.0f, maxHistory);
        result = lerp(history, raw, 1.0f / count);
    }

    gScreenGIOut[pixel] = result;
    gScreenGICountOut[pixel] = count;
}
