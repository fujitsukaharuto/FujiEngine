#include "../Common/DDGICommon.hlsli"

// プローブ1つ = 1グループ。各向きの物までの距離の平均と2乗の平均を貯める

ConstantBuffer<DDGIVolume> gDDGIVolume : register(b0);
Texture2D<float4> gDDGIRayData : register(t0);
RWTexture2D<float2> gDDGIDistanceOut : register(u0);

// 大きいほど texel の向きに近いレイだけを使う
static const float kDistanceSharpness = 50.0f;

static const uint kTileSize = kDDGIDistanceTexels + 2;

groupshared float3 gsDirection[kDDGIMaxRaysPerProbe];
groupshared float gsDistance[kDDGIMaxRaysPerProbe];
groupshared float2 gsResult[kDDGIDistanceTexels][kDDGIDistanceTexels];

[numthreads(kDDGIDistanceTexels, kDDGIDistanceTexels, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID, uint threadIndex : SV_GroupIndex)
{
    DDGIVolume volume = gDDGIVolume;
    uint probeIndex = DDGIUpdatedProbeIndex(volume, groupID.x);
    int3 slot = DDGIIndexToSlot(volume, probeIndex);
    uint2 tileOrigin = DDGISlotToTile(volume, slot) * kTileSize;

    const uint threadCount = kDDGIDistanceTexels * kDDGIDistanceTexels;
    // セルの対角線の1.5倍で切る (RTXGI と同じ)
    float maxDistance = volume.spacing * (1.7320508f * 1.5f);
    for (uint r = threadIndex; r < volume.raysPerProbe; r += threadCount)
    {
        // 裏面は負で入っている
        gsDistance[r] = min(abs(gDDGIRayData[uint2(kDDGIFixedRays + r, DDGIRayRow(volume, probeIndex))].a), maxDistance);
        gsDirection[r] = DDGIRayDirection(volume, r);
    }
    GroupMemoryBarrierWithGroupSync();

    float2 octUv = ((float2(threadID.xy) + 0.5f) / float(kDDGIDistanceTexels)) * 2.0f - 1.0f;
    float3 texelDirection = DDGIOctDecode(octUv);

    float2 sum = float2(0.0f, 0.0f);
    float weightSum = 0.0f;
    for (uint i = 0; i < volume.raysPerProbe; i++)
    {
        float w = pow(max(dot(texelDirection, gsDirection[i]), 0.0f), kDistanceSharpness);
        float d = gsDistance[i];
        sum += float2(d, d * d) * w;
        weightSum += w;
    }
    float2 moments = (weightSum > 1e-5f) ? sum / weightSum : float2(maxDistance, maxDistance * maxDistance);

    bool isReset = DDGIIsProbeReset(volume, slot);

    uint2 texel = tileOrigin + 1 + threadID.xy;
    float2 result = moments;
    if (!isReset)
    {
        result = lerp(moments, gDDGIDistanceOut[texel], volume.hysteresis);
    }
    gDDGIDistanceOut[texel] = result;
    gsResult[threadID.y][threadID.x] = result;
    GroupMemoryBarrierWithGroupSync();

    // 外周の縁 (DDGIUpdateIrradiance と同じ並べ方)
    const uint n = kDDGIDistanceTexels;
    const uint borderCount = 4 * n + 4;
    for (uint b = threadIndex; b < borderCount; b += threadCount)
    {
        uint2 dst;
        uint2 src;
        if (b < n)
        {
            dst = uint2(1 + b, 0);
            src = uint2(n - 1 - b, 0);
        }
        else if (b < 2 * n)
        {
            uint k = b - n;
            dst = uint2(1 + k, n + 1);
            src = uint2(n - 1 - k, n - 1);
        }
        else if (b < 3 * n)
        {
            uint k = b - 2 * n;
            dst = uint2(0, 1 + k);
            src = uint2(0, n - 1 - k);
        }
        else if (b < 4 * n)
        {
            uint k = b - 3 * n;
            dst = uint2(n + 1, 1 + k);
            src = uint2(n - 1, n - 1 - k);
        }
        else
        {
            uint k = b - 4 * n;
            dst = uint2((k & 1) ? n + 1 : 0, (k & 2) ? n + 1 : 0);
            src = uint2((k & 1) ? 0 : n - 1, (k & 2) ? 0 : n - 1);
        }
        gDDGIDistanceOut[tileOrigin + dst] = gsResult[src.y][src.x];
    }
}
