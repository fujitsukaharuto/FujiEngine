#include "../Object/Object3d.hlsli"

// 反射プローブを焼く。texel ごとに1本飛ばし、映り込みのレイの光らせ方に影なしのライトを全部足す
// 当たらなかった texel は a=0 で空は書かない。動く物は映さない

Texture2D<float4> gTextures[] : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b0);
#include "../Common/RayHitLighting.hlsli"
#include "../Common/IBLBake.hlsli"
#include "../Common/ClusteredLights.hlsli"

// ReflectionProbePass.h の CaptureParam と並びが一致していること
cbuffer ReflectionProbeCaptureParam : register(b1)
{
    float3 capturePosition;
    uint clusteredLightCount; // 影なしのライトの数。クラスタを使わず全部回す
    float maxDistance;
    uint captureSize;
    float textureLod;         // 当たった物のテクスチャを引くミップ
    float capturePad;
};

// 影なしのライト (ClusteredLightPass)
StructuredBuffer<ClusteredLight> gClusteredLights : register(t0, space10);

RWTexture2DArray<float4> gProbeCaptureOut : register(u0);

/// <summary>影なしのライトが当たった面を照らすぶん (Lambert)</summary>
float3 ClusteredLightsOnSurface(RayHitSurface surface)
{
    float3 light = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < clusteredLightCount; i++)
    {
        ClusteredLight clustered = gClusteredLights[i];
        // 形のある光源はいちばん近い点から
        float3 L;
        float distance;
        float attenuation = ClusteredLightShapeAttenuation(clustered, surface.position, L, distance);
        float facing = saturate(dot(surface.normal, L));
        if (attenuation <= 0.0f || facing <= 0.0f || distance <= 0.0f)
        {
            continue;
        }
        light += clustered.color * attenuation * (facing / kPI);
    }
    return light;
}

/// <summary>焼く位置から見て、不透明な物より手前のガラスを重ねる</summary>
void BlendCaptureGlass(float3 direction, float maxDistance, inout float4 color)
{
    RayDesc ray;
    ray.Origin = capturePosition;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskStaticTranslucent, ray);
    q.Proceed();
    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        return;
    }
    float glassDistance = max(q.CommittedRayT(), 1e-4f);
    uint instanceIndex = q.CommittedInstanceID() + q.CommittedGeometryIndex();
    RayHitSurface glass = LoadRayHitSurfaceLod(instanceIndex, q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                               q.CommittedObjectToWorld3x4(), capturePosition, direction, glassDistance, textureLod);
    float3 radiance = ShadeRayHit(glass, direction, (gLights.renderFlags & kRenderFlagGI) != 0);
    radiance += glass.albedo * ClusteredLightsOnSurface(glass);
    float opacity = saturate(gRayInstances[instanceIndex].opacity);
    // rgb は当たった割合を掛けてあるのでそのまま重ねる
    color = color * (1.0f - opacity) + float4(max(radiance, 0.0f), 1.0f) * opacity;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (dispatchID.x >= captureSize || dispatchID.y >= captureSize)
    {
        return;
    }
    float2 uv = (float2(dispatchID.xy) + 0.5f) / float(captureSize);
    float3 direction = DirectionFromCubeFace(dispatchID.z, uv);

    RayDesc ray;
    ray.Origin = capturePosition;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;

    // 不透明な物だけ
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskStaticOpaque, ray);
    q.Proceed();

    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        float4 sky = float4(0.0f, 0.0f, 0.0f, 0.0f);
        BlendCaptureGlass(direction, maxDistance, sky);
        gProbeCaptureOut[dispatchID] = sky;
        return;
    }
    // 裏面は焼く位置が物の中。黒い物とみなす
    if (!q.CommittedTriangleFrontFace())
    {
        gProbeCaptureOut[dispatchID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float hitDistance = max(q.CommittedRayT(), 1e-4f);
    RayHitSurface surface = LoadRayHitSurfaceLod(q.CommittedInstanceID() + q.CommittedGeometryIndex(),
                                                 q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                                 q.CommittedObjectToWorld3x4(), capturePosition, direction, hitDistance,
                                                 textureLod);
    bool useProbes = (gLights.renderFlags & kRenderFlagGI) != 0;
    float3 radiance = ShadeRayHit(surface, direction, useProbes);
    radiance += surface.albedo * ClusteredLightsOnSurface(surface);

    // 焼く位置から当たった所までの霧
    float fogAmount = FogAmount(gLights.fog, capturePosition, surface.position);
    if (fogAmount > 0.0f)
    {
        radiance = ApplyFog(radiance, FogColor(gLights.fog, direction), fogAmount, 0.0f);
    }
    float4 color = float4(max(radiance, 0.0f), 1.0f);
    BlendCaptureGlass(direction, hitDistance, color);
    gProbeCaptureOut[dispatchID] = color;
}
