#include "../Common/ScreenProbeCommon.hlsli"

// 画素ごとに近くのプローブから間接光を引く。同じ面で向きの近いプローブだけを混ぜる
// a は使えるプローブがあったか。近くの遮蔽は深度から見て間接光に掛ける (Lumen の Short Range AO)

Texture2D<float4> gProbeSHFiltered : register(t7, space7);
Texture2D<float4> gProbePosition : register(t5, space7);
Texture2D<float4> gProbeNormal : register(t6, space7);
RWTexture2D<float4> gScreenGIRawOut : register(u0);

// これ以上自分の面から離れたプローブは使わない (距離に対する割合)
static const float kPlaneTolerance = 0.03f;
// 画面上の距離の重みの広がり (プローブの間隔が1)
static const float kSpatialSigma = 0.6f;
// 近くの遮蔽を探す範囲(m)と本数
static const float kShortRangeAORadius = 0.5f;
static const uint kShortRangeAOSamples = 8;
// 画面上でこれより広く探さない
static const float kShortRangeAOMaxPixels = 48.0f;

/// <summary>深度から見た、近くの物による遮蔽。1で開けている</summary>
float ShortRangeAO(uint2 pixel, float3 position, float3 normal, float viewDistance)
{
    float radiusPixels = min(kShortRangeAORadius * pixelsPerUnit / max(viewDistance, 1e-3f), kShortRangeAOMaxPixels);
    if (radiusPixels < 1.0f)
    {
        return 1.0f;
    }
    // interleaved gradient noise
    float2 noisePixel = float2(pixel) + 5.588238f * float(frameIndex & 63u);
    float noise = frac(52.9829189f * frac(dot(noisePixel, float2(0.06711056f, 0.00583715f))));

    float occlusion = 0.0f;
    for (uint i = 0; i < kShortRangeAOSamples; i++)
    {
        // 黄金角の渦巻き
        float angle = (float(i) + noise) * 2.39996323f;
        float radius = sqrt((float(i) + 0.5f) / float(kShortRangeAOSamples)) * radiusPixels;
        int2 tap = int2(pixel) + int2(round(float2(cos(angle), sin(angle)) * radius));
        if (any(tap < 0) || any(tap >= int2(screenSize)))
        {
            continue;
        }
        float tapDepth = gGBufferDepth.Load(int3(tap, 0));
        if (tapDepth >= 1.0f)
        {
            continue;
        }
        float3 toTap = ReconstructWorldPos(uint2(tap), screenSize, tapDepth, invViewProj) - position;
        float tapDistance = length(toTap);
        if (tapDistance < 1e-4f)
        {
            continue;
        }
        // 面より上の近い物ほど遮る
        float cosine = dot(normal, toTap) / tapDistance;
        occlusion += saturate(cosine - 0.15f) * saturate(1.0f - tapDistance / kShortRangeAORadius);
    }
    return saturate(1.0f - occlusion * (2.0f / float(kShortRangeAOSamples)));
}

// 外側の輪まで探すときの距離の重みの広がり
static const float kWideSpatialSigma = 1.2f;
// これより面の一致が弱ければ外側の輪まで探す
static const float kWideSearchQuality = 0.5f;

/// <summary>プローブ1つぶんを足し込む。同じ面に乗っていて向きの近いものほど重い</summary>
void AccumulateProbe(int2 probe, float2 probeCoord, float sigma, float3 position, float3 normal, float tolerance,
                     inout float3 irradiance, inout float weightSum, inout float quality)
{
    if (any(probe < 0) || any(probe >= int2(probeCount)))
    {
        return;
    }
    float4 probePosition = gProbePosition.Load(int3(probe, 0));
    if (probePosition.w <= 0.0f)
    {
        return;
    }
    float3 probeNormal = gProbeNormal.Load(int3(probe, 0)).xyz;
    float planeDistance = abs(dot(normal, probePosition.xyz - position));
    float surfaceWeight = exp(-planeDistance / tolerance) * pow(saturate(dot(normal, probeNormal)), 8.0f);
    if (surfaceWeight < 1e-3f)
    {
        return;
    }
    float2 offset = float2(probe) - probeCoord;
    float weight = exp(-dot(offset, offset) / (2.0f * sigma * sigma)) * surfaceWeight;

    float3 probeIrradiance;
    probeIrradiance.r = SHIrradiance(gProbeSHFiltered.Load(int3(probe.x * 3 + 0, probe.y, 0)), normal);
    probeIrradiance.g = SHIrradiance(gProbeSHFiltered.Load(int3(probe.x * 3 + 1, probe.y, 0)), normal);
    probeIrradiance.b = SHIrradiance(gProbeSHFiltered.Load(int3(probe.x * 3 + 2, probe.y, 0)), normal);
    irradiance += probeIrradiance * weight;
    weightSum += weight;
    quality = max(quality, surfaceWeight);
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
        gScreenGIRawOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float3 position = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
    float viewDistance = distance(position, cameraPos);
    float tolerance = max(kPlaneTolerance * viewDistance, 0.02f);

    // プローブの間隔を1とした画面上の位置。プローブ i の基準はタイルの中心
    float2 probeCoord = (float2(pixel) + 0.5f) / float(tileSize) - 0.5f;
    int2 nearest = int2(round(probeCoord));

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    float quality = 0.0f;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            AccumulateProbe(nearest + int2(x, y), probeCoord, kSpatialSigma, position, normal, tolerance,
                            irradiance, weightSum, quality);
        }
    }
    // 物の縁でちらつかないよう、外側の輪まで探して同じ面のものを拾う
    if (quality < kWideSearchQuality)
    {
        for (int ry = -2; ry <= 2; ry++)
        {
            for (int rx = -2; rx <= 2; rx++)
            {
                if (abs(rx) == 2 || abs(ry) == 2)
                {
                    AccumulateProbe(nearest + int2(rx, ry), probeCoord, kWideSpatialSigma, position, normal, tolerance,
                                    irradiance, weightSum, quality);
                }
            }
        }
    }

    if (weightSum <= 1e-5f)
    {
        gScreenGIRawOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float ao = ShortRangeAO(pixel, position, normal, viewDistance);
    gScreenGIRawOut[pixel] = float4(irradiance / weightSum * ao, saturate(quality * 2.0f));
}
