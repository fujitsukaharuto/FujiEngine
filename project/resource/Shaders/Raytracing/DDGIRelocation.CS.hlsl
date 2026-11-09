#include "../Common/DDGICommon.hlsli"

// プローブの再配置 (RTXGI の DDGIProbeRelocationCS)。埋まったプローブと壁に近すぎるプローブをずらす
// 近くに何も無ければセルの中心へ戻す。ずらすのはセルの 0.45 倍まで

ConstantBuffer<DDGIVolume> gDDGIVolume : register(b0);
Texture2D<float4> gDDGIRayData : register(t0);
RWTexture2D<float4> gDDGIProbeDataOut : register(u0);

// 裏面に当たったレイがこの割合を超えたら埋まっているとみなす。DDGIUpdateIrradiance と同じ値
static const float kBackfaceThreshold = 0.25f;
// 表の面がプローブの間隔のこの割合より近ければ離す
static const float kMinFrontfaceDistance = 0.2f;

[numthreads(32, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    DDGIVolume volume = gDDGIVolume;
    uint probeCount = uint(volume.probeCounts.x * volume.probeCounts.y * volume.probeCounts.z);
    uint probeIndex = DDGIUpdatedProbeIndex(volume, dispatchID.x);
    if (probeIndex >= probeCount)
    {
        return;
    }

    int3 slot = DDGIIndexToSlot(volume, probeIndex);
    uint2 texel = DDGISlotToTile(volume, slot);
    if (DDGIIsProbeReset(volume, slot))
    {
        gDDGIProbeDataOut[texel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float3 offset = gDDGIProbeDataOut[texel].xyz;
    float minFrontfaceDistance = volume.spacing * kMinFrontfaceDistance;

    int closestBackfaceIndex = -1;
    int closestFrontfaceIndex = -1;
    int farthestFrontfaceIndex = -1;
    float closestBackfaceDistance = 1e27f;
    float closestFrontfaceDistance = 1e27f;
    float farthestFrontfaceDistance = 0.0f;
    uint backfaceCount = 0;

    for (uint r = 0; r < kDDGIFixedRays; r++)
    {
        float hitDistance = gDDGIRayData[uint2(r, DDGIRayRow(volume, probeIndex))].a;
        if (hitDistance < 0.0f)
        {
            // 裏面は -距離*0.2 で入っている (DDGITrace)
            ++backfaceCount;
            hitDistance *= -5.0f;
            if (hitDistance < closestBackfaceDistance)
            {
                closestBackfaceDistance = hitDistance;
                closestBackfaceIndex = int(r);
            }
        }
        else
        {
            if (hitDistance < closestFrontfaceDistance)
            {
                closestFrontfaceDistance = hitDistance;
                closestFrontfaceIndex = int(r);
            }
            if (hitDistance > farthestFrontfaceDistance)
            {
                farthestFrontfaceDistance = hitDistance;
                farthestFrontfaceIndex = int(r);
            }
        }
    }

    float3 fullOffset = float3(1e27f, 1e27f, 1e27f);
    if (closestBackfaceIndex != -1 && float(backfaceCount) / float(kDDGIFixedRays) > kBackfaceThreshold)
    {
        // 埋まっている。いちばん近い裏面を少し越えた所へ
        fullOffset = offset + DDGIFixedRayDirection(uint(closestBackfaceIndex)) * (closestBackfaceDistance * 1.5f);
    }
    else if (closestFrontfaceIndex != -1 && closestFrontfaceDistance < minFrontfaceDistance)
    {
        // 表の面に近すぎる。いちばん遠い面の側へ、その面を越えない範囲で離す
        float3 closestDirection = DDGIFixedRayDirection(uint(closestFrontfaceIndex));
        float3 farthestDirection = DDGIFixedRayDirection(uint(max(farthestFrontfaceIndex, 0)));
        if (dot(closestDirection, farthestDirection) <= 0.0f)
        {
            fullOffset = offset + farthestDirection * min(farthestFrontfaceDistance, volume.spacing * 0.33f);
        }
    }
    else if (closestFrontfaceDistance > minFrontfaceDistance)
    {
        // 近くに何も無い。セルの中心へ戻していく
        float offsetLength = length(offset);
        if (offsetLength > 1e-5f)
        {
            float moveBack = min(closestFrontfaceDistance - minFrontfaceDistance, offsetLength);
            fullOffset = offset - (offset / offsetLength) * moveBack;
        }
    }

    // 楕円体 (半径 0.45 セル) の内側だけ受け付ける
    float3 normalizedOffset = fullOffset / volume.spacing;
    if (dot(normalizedOffset, normalizedOffset) < 0.2025f)
    {
        offset = fullOffset;
    }
    gDDGIProbeDataOut[texel] = float4(offset, 0.0f);
}
