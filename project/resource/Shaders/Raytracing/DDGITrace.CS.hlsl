#include "../Object/Object3d.hlsli"

// プローブごとに全方位へレイを飛ばし、当たった先の明るさと距離を書き出す

Texture2D<float4> gTextures[] : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b0);
#include "../Common/RayHitLighting.hlsli"

// x: 放射輝度, w: 距離。裏面なら負
RWTexture2D<float4> gDDGIRayData : register(u0);
// 今レイを飛ばしている格子
ConstantBuffer<DDGIVolume> gDDGIVolume : register(b5);

[numthreads(32, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID)
{
    DDGIVolume volume = gDDGIVolume;
    // 先頭の kDDGIFixedRays 本は向きを回さないレイ
    uint rayIndex = groupID.x * 32 + threadID.x;
    uint probeIndex = DDGIUpdatedProbeIndex(volume, groupID.y);
    if (rayIndex >= kDDGIFixedRays + volume.raysPerProbe)
    {
        return;
    }

    int3 slot = DDGIIndexToSlot(volume, probeIndex);
    int3 cell = DDGISlotToCell(volume.baseCell, volume.probeCounts, slot);
    float3 origin = DDGICellToPosition(volume, cell);
    // 作り直すプローブのずらし量は使わない
    if (!DDGIIsProbeReset(volume, slot))
    {
        origin += gDDGIProbeData.Load(int3(DDGISlotToTile(volume, slot), 0)).xyz;
    }
    float3 direction = (rayIndex < kDDGIFixedRays) ? DDGIFixedRayDirection(rayIndex)
                                                   : DDGIRayDirection(volume, rayIndex - kDDGIFixedRays);

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = volume.maxRayDistance;

    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
    q.Proceed();

    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        gDDGIRayData[uint2(rayIndex, DDGIRayRow(volume, probeIndex))] = float4(RayMissRadiance(direction), volume.maxRayDistance);
        return;
    }

    float hitDistance = q.CommittedRayT();
    // 裏面に当たればプローブが埋まっている
    if (!q.CommittedTriangleFrontFace())
    {
        gDDGIRayData[uint2(rayIndex, DDGIRayRow(volume, probeIndex))] = float4(0.0f, 0.0f, 0.0f, -hitDistance * 0.2f);
        return;
    }

    RayHitSurface surface = LoadRayHitSurface(q.CommittedInstanceID() + q.CommittedGeometryIndex(),
                                              q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                              q.CommittedObjectToWorld3x4(), origin, direction, hitDistance);
    // 前のフレームまでの間接光で跳ね返りを重ねる。作り直した直後は足さない
    float3 radiance = ShadeRayHit(surface, direction, volume.resetAll == 0);
    gDDGIRayData[uint2(rayIndex, DDGIRayRow(volume, probeIndex))] = float4(radiance, hitDistance);
}
