#include "../Object/Object3d.hlsli"

// 焼く GI (BakedGIPass) のレイ。並びは DDGITrace と同じで、光は成分ごとに求める
// 動く物・発光・点光源は焼かない

Texture2D<float4> gTextures[] : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b0);
#include "../Common/RayHitLighting.hlsli"

RWTexture2D<float4> gDDGIRayData : register(u0);
ConstantBuffer<DDGIVolume> gDDGIVolume : register(b5);

/// <summary>何にも当たらなかったレイが運ぶ、その成分の明るさ</summary>
/// <remarks>空は HemisphereAmbient と同じ割合で上と下の色に分ける</remarks>
float3 BakeMissRadiance(DDGIVolume volume, float3 direction)
{
    float up = direction.y * 0.5f + 0.5f;
    if (volume.bakeComponent == kDDGIBakeSkyUp)
    {
        return up.xxx;
    }
    if (volume.bakeComponent == kDDGIBakeSkyDown)
    {
        return (1.0f - up).xxx;
    }
    return float3(0.0f, 0.0f, 0.0f);
}

/// <summary>太陽が配置物に遮られずに届くか</summary>
bool IsSunVisible(float3 position, float3 normal, float3 toSun)
{
    RayDesc ray;
    ray.Origin = position + normal * kShadowRayBias;
    ray.Direction = toSun;
    ray.TMin = 0.0f;
    ray.TMax = kShadowRayMaxDistance;
    // 抜きのある物も板として遮る
    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskStaticOpaque, ray);
    q.Proceed();
    return q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
}

/// <summary>当たった面からレイの原点へ返る、その成分の明るさ</summary>
/// <param name="useProbes">焼きかけの値から跳ね返りを足すか</param>
float3 ShadeBakeHit(DDGIVolume volume, RayHitSurface surface, float3 direction, bool useProbes)
{
    float3 light = float3(0.0f, 0.0f, 0.0f);
    if (volume.bakeComponent == kDDGIBakeSun)
    {
        float facing = saturate(dot(surface.normal, volume.bakeToSun));
        if (facing > 0.0f && IsSunVisible(surface.position, surface.normal, volume.bakeToSun))
        {
            light += facing / kPI;
        }
    }

    // 箱の外の面は、空の成分なら空の明るさ、太陽の成分なら跳ね返り無し
    float3 outside = BakeMissRadiance(volume, surface.normal);
    if (useProbes)
    {
        float giWeight;
        float3 indirect = DDGIGetIrradiance(surface.position, surface.normal, -direction, gSampler, giWeight);
        light += lerp(outside, indirect, giWeight);
    }
    else
    {
        light += outside;
    }
    return surface.albedo * light;
}

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
    // 再配置を始めた後の成分はずらした位置から飛ばす
    if ((volume.bakeFlags & kDDGIBakeFlagUseOffsets) != 0)
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
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskStaticOpaque | kRayMaskStaticTranslucent, ray);
    q.Proceed();

    uint2 output = uint2(rayIndex, DDGIRayRow(volume, probeIndex));
    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        gDDGIRayData[output] = float4(BakeMissRadiance(volume, direction), volume.maxRayDistance);
        return;
    }

    float hitDistance = q.CommittedRayT();
    // 裏面に当たればプローブが埋まっている
    if (!q.CommittedTriangleFrontFace())
    {
        gDDGIRayData[output] = float4(0.0f, 0.0f, 0.0f, -hitDistance * 0.2f);
        return;
    }

    RayHitSurface surface = LoadRayHitSurface(q.CommittedInstanceID() + q.CommittedGeometryIndex(),
                                              q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                              q.CommittedObjectToWorld3x4(), origin, direction, hitDistance);
    gDDGIRayData[output] = float4(ShadeBakeHit(volume, surface, direction, volume.resetAll == 0), hitDistance);
}
