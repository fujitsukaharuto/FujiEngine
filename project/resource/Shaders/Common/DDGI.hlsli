#ifndef DDGI_HLSLI
#define DDGI_HLSLI

// プローブの間接光を引く側。前方描画と、2回目以降の跳ね返りが使う
#include "DDGICommon.hlsli"

// space0〜5 は使用済みなので space6
Texture2D<float4> gDDGIIrradiance : register(t0, space6);
Texture2D<float2> gDDGIDistance : register(t1, space6);
// プローブごとに1texel。xyz は再配置のずれ
Texture2D<float4> gDDGIProbeData : register(t2, space6);
struct DDGIVolumes
{
    DDGIVolume volumes[kDDGIVolumeCount];
};
ConstantBuffer<DDGIVolumes> gDDGIVolumes : register(b4);

/// <summary>1つの格子から引いた、その点に回り込む間接光</summary>
/// <param name="worldPos">面の位置</param>
/// <param name="normal">面の法線</param>
/// <param name="toViewer">面から見ている側へ向かう単位ベクトル</param>
/// <param name="smp">サンプラー</param>
/// <param name="weight">格子の中にいる度合い(0〜1)。位置だけで決まる</param>
float3 DDGIGetVolumeIrradiance(DDGIVolume volume, float3 worldPos, float3 normal, float3 toViewer, SamplerState smp, out float weight)
{
    weight = 0.0f;

    // 面から少し浮かせた点で引く
    float3 biasedPos = worldPos + (normal * volume.normalBias + toViewer * volume.viewBias);
    float3 gridPos = biasedPos / volume.spacing - 0.5f - float3(volume.baseCell);
    float3 lastIndex = float3(volume.probeCounts - 1);
    if (any(gridPos < 0.0f) || any(gridPos > lastIndex))
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    // 格子の端の1セルで元のアンビエントへ溶かす
    float3 edge = saturate(min(gridPos, lastIndex - gridPos));
    float volumeWeight = edge.x * edge.y * edge.z;

    int3 baseCell = volume.baseCell + int3(min(floor(gridPos), lastIndex - 1.0f));
    float3 alpha = saturate(gridPos - float3(baseCell - volume.baseCell));

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    for (uint i = 0; i < 8; i++)
    {
        int3 offset = int3(i & 1, (i >> 1) & 1, (i >> 2) & 1);
        int3 cell = baseCell + offset;
        int3 slot = DDGICellToSlot(volume, cell);
        uint2 tile = DDGISlotToTile(volume, slot);
        float3 probePos = DDGICellToPosition(volume, cell) + gDDGIProbeData.Load(int3(tile, 0)).xyz;

        float3 trilinear = lerp(1.0f - alpha, alpha, float3(offset));
        float w = trilinear.x * trilinear.y * trilinear.z;

        // 面の裏にあるプローブはほぼ使わない (RTXGI は 0.2 残す)。割合で割り戻すので黒くはならない
        float3 toProbe = normalize(probePos - worldPos);
        float backface = (dot(toProbe, normal) + 1.0f) * 0.5f;
        float probeWeight = backface * backface + 0.02f;

        // 物陰に隠れていないか (チェビシェフの不等式)
        float3 probeToPoint = biasedPos - probePos;
        float distance = length(probeToPoint);
        float2 moments = gDDGIDistance.SampleLevel(smp,
            DDGIAtlasUv(tile, probeToPoint / max(distance, 1e-4f), kDDGIDistanceTexels, volume.distanceAtlasSize), 0.0f);
        if (distance > moments.x)
        {
            float variance = abs(moments.x * moments.x - moments.y);
            float difference = distance - moments.x;
            float chebyshev = variance / (variance + difference * difference);
            probeWeight *= max(chebyshev * chebyshev * chebyshev, 0.05f);
        }

        // 小さい重みをさらに潰す
        probeWeight = max(probeWeight, 1e-6f);
        const float crushThreshold = 0.2f;
        if (probeWeight < crushThreshold)
        {
            probeWeight *= probeWeight * probeWeight / (crushThreshold * crushThreshold);
        }

        float4 probe = gDDGIIrradiance.SampleLevel(smp,
            DDGIAtlasUv(DDGIIrradianceTile(volume, slot), normal, kDDGIIrradianceTexels, volume.irradianceAtlasSize), 0.0f);
        // a は使えるプローブか
        probeWeight *= w * probe.a;

        // 符号化を戻す。2乗ぶんは残して混ぜる (RTXGI と同じ)
        irradiance += pow(max(probe.rgb, 0.0f), kDDGIIrradianceGamma * 0.5f) * probeWeight;
        totalWeight += probeWeight;
    }

    weight = volumeWeight;
    if (totalWeight <= 1e-5f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    irradiance /= totalWeight;
    return irradiance * irradiance;
}

/// <summary>その点に回り込む間接光。アンビエントと同じ尺度</summary>
/// <remarks>細かい格子を優先し、その端と外では1段粗い格子へ溶かす</remarks>
/// <param name="weight">どれかの格子の中にいる度合い(0〜1)</param>
float3 DDGIGetIrradiance(float3 worldPos, float3 normal, float3 toViewer, SamplerState smp, out float weight)
{
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    weight = 0.0f;
    // まだ細かい格子で埋まっていない割合
    float remaining = 1.0f;
    [loop]
    for (int v = int(kDDGIVolumeCount) - 1; v >= 0 && remaining > 0.0f; v--)
    {
        float volumeWeight;
        float3 volumeIrradiance = DDGIGetVolumeIrradiance(gDDGIVolumes.volumes[v], worldPos, normal, toViewer, smp, volumeWeight);
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

#endif // DDGI_HLSLI
