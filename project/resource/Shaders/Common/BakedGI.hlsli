#ifndef BAKED_GI_HLSLI
#define BAKED_GI_HLSLI

// 焼いた間接光 (BakedGIPass) を引く側。成分ごとに今の空と太陽の色を掛けて足す
#include "DDGICommon.hlsli"

// space0〜8 は使用済みなので space9
Texture2D<float4> gBakedSky : register(t0, space9);       // 空の上の色の段、下の色の段を縦へ積む
Texture2D<float4> gBakedSun : register(t1, space9);       // 太陽を焼いた時刻ごとの段
Texture2D<float2> gBakedDistance : register(t2, space9);
Texture2D<float4> gBakedProbeData : register(t3, space9);

// BakedGIPass.h の BakedGIParams と並びが一致していること
static const uint kBakedGIMaxVolumes = 3;
struct BakedGIParams
{
    DDGIVolume volumes[kBakedGIMaxVolumes]; // irradianceRowOffset は 0
    uint volumeCount;   // 0 なら焼いた GI は無い
    uint skyDownRow;    // 空の下の色の段の、タイルの行
    uint sunRowA;       // 今の時刻の前後の太陽の段
    uint sunRowB;
    float3 sunRadiance; // 太陽の色と強さ。夜は0
    float sunBlend;     // sunRowA から sunRowB へ混ぜる割合
    float2 sunAtlasSize;
    float2 bakedPad;
};
ConstantBuffer<BakedGIParams> gBakedGI : register(b6);

/// <summary>1つの箱から引いた、成分ごとの間接光</summary>
/// <param name="weight">箱の中にいる度合い(0〜1)。外なら0</param>
float3 BakedGIGetVolumeIrradiance(DDGIVolume volume, float3 worldPos, float3 normal, float3 toViewer,
                                  float3 skyUpColor, float3 skyDownColor, SamplerState smp, out float weight)
{
    weight = 0.0f;

    float3 biasedPos = worldPos + (normal * volume.normalBias + toViewer * volume.viewBias);
    float3 gridPos = biasedPos / volume.spacing - 0.5f - float3(volume.baseCell);
    float3 lastIndex = float3(volume.probeCounts - 1);
    if (any(gridPos < 0.0f) || any(gridPos > lastIndex))
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    // 箱の端の1セルで元のアンビエントへ溶かす
    float3 edge = saturate(min(gridPos, lastIndex - gridPos));
    float volumeWeight = edge.x * edge.y * edge.z;

    int3 baseCell = volume.baseCell + int3(min(floor(gridPos), lastIndex - 1.0f));
    float3 alpha = saturate(gridPos - float3(baseCell - volume.baseCell));

    float3 skyUp = float3(0.0f, 0.0f, 0.0f);
    float3 skyDown = float3(0.0f, 0.0f, 0.0f);
    float3 sunA = float3(0.0f, 0.0f, 0.0f);
    float3 sunB = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    const bool hasSun = any(gBakedGI.sunRadiance > 0.0f);
    for (uint i = 0; i < 8; i++)
    {
        int3 offset = int3(i & 1, (i >> 1) & 1, (i >> 2) & 1);
        int3 cell = baseCell + offset;
        int3 slot = DDGICellToSlot(volume, cell);
        uint2 tile = DDGISlotToTile(volume, slot);
        float3 probePos = DDGICellToPosition(volume, cell) + gBakedProbeData.Load(int3(tile, 0)).xyz;

        float3 trilinear = lerp(1.0f - alpha, alpha, float3(offset));
        float w = trilinear.x * trilinear.y * trilinear.z;

        // 重みの付け方は DDGI.hlsli と同じ
        float3 toProbe = normalize(probePos - worldPos);
        float backface = (dot(toProbe, normal) + 1.0f) * 0.5f;
        float probeWeight = backface * backface + 0.02f;

        float3 probeToPoint = biasedPos - probePos;
        float distance = length(probeToPoint);
        float2 moments = gBakedDistance.SampleLevel(smp,
            DDGIAtlasUv(tile, probeToPoint / max(distance, 1e-4f), kDDGIDistanceTexels, volume.distanceAtlasSize), 0.0f);
        if (distance > moments.x)
        {
            float variance = abs(moments.x * moments.x - moments.y);
            float difference = distance - moments.x;
            float chebyshev = variance / (variance + difference * difference);
            probeWeight *= max(chebyshev * chebyshev * chebyshev, 0.05f);
        }

        probeWeight = max(probeWeight, 1e-6f);
        const float crushThreshold = 0.2f;
        if (probeWeight < crushThreshold)
        {
            probeWeight *= probeWeight * probeWeight / (crushThreshold * crushThreshold);
        }

        float4 up = gBakedSky.SampleLevel(smp, DDGIAtlasUv(tile, normal, kDDGIIrradianceTexels, volume.irradianceAtlasSize), 0.0f);
        // a は使えるプローブか
        probeWeight *= w * up.a;
        if (probeWeight <= 0.0f)
        {
            continue;
        }
        float4 down = gBakedSky.SampleLevel(smp,
            DDGIAtlasUv(tile + uint2(0, gBakedGI.skyDownRow), normal, kDDGIIrradianceTexels, volume.irradianceAtlasSize), 0.0f);

        // 符号化を戻す。2乗ぶんは残して混ぜる (DDGI.hlsli と同じ)
        const float decode = kDDGIIrradianceGamma * 0.5f;
        skyUp += pow(max(up.rgb, 0.0f), decode) * probeWeight;
        skyDown += pow(max(down.rgb, 0.0f), decode) * probeWeight;
        if (hasSun)
        {
            float4 a = gBakedSun.SampleLevel(smp,
                DDGIAtlasUv(tile + uint2(0, gBakedGI.sunRowA), normal, kDDGIIrradianceTexels, gBakedGI.sunAtlasSize), 0.0f);
            float4 b = gBakedSun.SampleLevel(smp,
                DDGIAtlasUv(tile + uint2(0, gBakedGI.sunRowB), normal, kDDGIIrradianceTexels, gBakedGI.sunAtlasSize), 0.0f);
            sunA += pow(max(a.rgb, 0.0f), decode) * probeWeight;
            sunB += pow(max(b.rgb, 0.0f), decode) * probeWeight;
        }
        totalWeight += probeWeight;
    }

    weight = volumeWeight;
    if (totalWeight <= 1e-5f)
    {
        // 見通せるプローブが無ければ空の光で埋めない (DDGI.hlsli と同じ)
        return float3(0.0f, 0.0f, 0.0f);
    }
    float inverse = 1.0f / totalWeight;
    skyUp *= inverse;
    skyDown *= inverse;
    float3 irradiance = skyUp * skyUp * skyUpColor + skyDown * skyDown * skyDownColor;
    if (hasSun)
    {
        sunA *= inverse;
        sunB *= inverse;
        irradiance += lerp(sunA * sunA, sunB * sunB, gBakedGI.sunBlend) * gBakedGI.sunRadiance;
    }
    return irradiance;
}

/// <summary>その点に回り込む、焼いた間接光</summary>
/// <remarks>箱が重なれば後ろの番号を優先し、その端で前の番号へ溶かす</remarks>
/// <param name="skyUpColor">真上を向いた面のアンビエント</param>
/// <param name="skyDownColor">真下を向いた面のアンビエント</param>
/// <param name="weight">どれかの箱の中にいる度合い(0〜1)</param>
float3 BakedGIGetIrradiance(float3 worldPos, float3 normal, float3 toViewer, float3 skyUpColor, float3 skyDownColor,
                            SamplerState smp, out float weight)
{
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    weight = 0.0f;
    float remaining = 1.0f;
    [loop]
    for (int v = int(gBakedGI.volumeCount) - 1; v >= 0 && remaining > 0.0f; v--)
    {
        float volumeWeight;
        float3 volumeIrradiance = BakedGIGetVolumeIrradiance(gBakedGI.volumes[v], worldPos, normal, toViewer,
                                                             skyUpColor, skyDownColor, smp, volumeWeight);
        irradiance += volumeIrradiance * (remaining * volumeWeight);
        weight += remaining * volumeWeight;
        remaining *= 1.0f - volumeWeight;
    }
    if (weight <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    return irradiance / weight;
}

#endif // BAKED_GI_HLSLI
