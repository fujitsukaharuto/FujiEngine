#ifndef RAY_HIT_LIGHTING_HLSLI
#define RAY_HIT_LIGHTING_HLSLI

// 間接光のレイが当たった先を光らせる共通処理。gLights / gTextures[] / gSampler の宣言の後に include する
#include "IBL.hlsli"
#include "DDGI.hlsli"

// 空の代わりに引く、環境マップのぼかしたミップ
static const float kRayHitSkyMip = 2.0f;

/// <summary>レイが何にも当たらなかったときの明るさ</summary>
float3 RayMissRadiance(float3 direction)
{
    if (gLights.ambientMode == kAmbientModeIBL && gLights.hasEnvironment != 0)
    {
        return gPrefilteredEnv.SampleLevel(gSampler, direction, kRayHitSkyMip).rgb * gLights.environmentTint
               * (gLights.ambientIntensity * gLights.iblAmbientScale);
    }
    return HemisphereAmbient(direction, gLights.ambientSkyColor, gLights.ambientGroundColor, gLights.ambientIntensity);
}

/// <summary>レイが当たった面の位置・向き・色</summary>
struct RayHitSurface
{
    float3 position;
    float3 normal;   // レイの来た側を向く
    float3 albedo;
    float3 emission;
    float roughness; // 映り込みを付けていないマテリアルは1
    float metallic;
};

/// <summary>RayQuery の当たりから面を引く</summary>
/// <param name="instanceIndex">CommittedInstanceID() + CommittedGeometryIndex()</param>
/// <param name="textureLod">色と発光のテクスチャを引くミップ</param>
RayHitSurface LoadRayHitSurfaceLod(uint instanceIndex, uint primitiveIndex, float2 barycentrics, float3x4 objectToWorld,
                                   float3 origin, float3 direction, float hitDistance, float textureLod)
{
    RayInstance instance = gRayInstances[instanceIndex];
    RayHitSurface surface;
    surface.position = origin + direction * hitDistance;
    surface.normal = -direction;
    float2 uv = float2(0.5f, 0.5f);
    bool hasUV = false;
    if ((instance.flags & kRayInstanceHasGeometry) != 0)
    {
        uint first = primitiveIndex * 3;
        Buffer<uint> indices = gRayIndices[NonUniformResourceIndex(instance.indexBuffer)];
        StructuredBuffer<RayVertex> vertices = gRayVertices[NonUniformResourceIndex(instance.vertexBuffer)];
        RayVertex v0 = vertices[indices[first + 0]];
        RayVertex v1 = vertices[indices[first + 1]];
        RayVertex v2 = vertices[indices[first + 2]];
        float3 w = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
        uv = ((v0.texcoord * w.x + v1.texcoord * w.y + v2.texcoord * w.z) + instance.uvOffset) * instance.uvScale;
        hasUV = true;
        if ((instance.flags & kRayInstanceHasNormal) != 0)
        {
            float3 objectNormal = v0.normal * w.x + v1.normal * w.y + v2.normal * w.z;
            float3 worldNormal = mul((float3x3) objectToWorld, objectNormal);
            if (dot(worldNormal, worldNormal) > 1e-8f)
            {
                worldNormal = normalize(worldNormal);
                surface.normal = dot(worldNormal, direction) > 0.0f ? -worldNormal : worldNormal;
            }
        }
    }
    float3 textureColor = gTextures[NonUniformResourceIndex(instance.textureIndex)]
                              .SampleLevel(gSampler, uv, hasUV ? textureLod : 16.0f).rgb;
    surface.albedo = instance.color * textureColor;
    // 発光は発光テクスチャが決める。無い物は色のテクスチャ
    surface.emission = float3(0.0f, 0.0f, 0.0f);
    if (any(instance.emissive > 0.0f))
    {
        surface.emission = instance.emissive * gTextures[NonUniformResourceIndex(instance.emissiveTextureIndex)]
                                                    .SampleLevel(gSampler, uv, hasUV ? textureLod : 16.0f).rgb;
    }
    surface.roughness = instance.roughness;
    surface.metallic = saturate(instance.metallic);
    return surface;
}

/// <summary>RayQuery の当たりから面を引く。色はぼかしたミップ</summary>
RayHitSurface LoadRayHitSurface(uint instanceIndex, uint primitiveIndex, float2 barycentrics, float3x4 objectToWorld,
                                float3 origin, float3 direction, float hitDistance)
{
    return LoadRayHitSurfaceLod(instanceIndex, primitiveIndex, barycentrics, objectToWorld, origin, direction, hitDistance, 3.0f);
}

/// <summary>当たった面からレイの原点へ返る明るさ</summary>
/// <param name="direction">飛ばしたレイの向き</param>
/// <param name="useProbes">格子のプローブの間接光を足すか。中身が無いときは空で代える</param>
float3 ShadeRayHit(RayHitSurface surface, float3 direction, bool useProbes)
{
    float3 light = float3(0.0f, 0.0f, 0.0f);
    if (gLights.numDirectionalLights > 0)
    {
        DirectionalLight sun = gLights.directionalLights[0];
        float3 L = normalize(-sun.direction);
        float facing = saturate(dot(surface.normal, L));
        float3 radiance = sun.color.rgb * sun.intensity;
        if (facing > 0.0f && any(radiance > 0.0f))
        {
            float shadow = TraceShadowRay(surface.position, L, surface.normal, kShadowRayMaxDistance);
            light += radiance * (facing * shadow / kPI);
        }
    }

    // 影の有無は前方描画と同じビットに従う
    for (int j = 0; j < gLights.numPointLights; j++)
    {
        PointLight pointLight = gLights.pointLights[j];
        float3 L = normalize(pointLight.position - surface.position);
        float facing = saturate(dot(surface.normal, L));
        float3 radiance = pointLight.color.rgb * pointLight.intensity * PointLightAttenuation(pointLight, surface.position);
        if (facing <= 0.0f || !any(radiance > 0.0f))
        {
            continue;
        }
        radiance *= TraceShadowPunctual(surface.position, pointLight.position, surface.normal,
                                        gLights.rayTracedShadowMask, kShadowMaskPoint, pointLight.sourceRadius);
        light += radiance * (facing / kPI);
    }
    for (int k = 0; k < gLights.numSpotLights; k++)
    {
        SpotLight spotLight = gLights.spotLights[k];
        float3 lightDirOnSurface = normalize(surface.position - spotLight.position);
        float facing = saturate(dot(surface.normal, -lightDirOnSurface));
        float3 radiance = spotLight.color.rgb * spotLight.intensity * SpotLightAttenuation(spotLight, surface.position, lightDirOnSurface);
        if (facing <= 0.0f || !any(radiance > 0.0f))
        {
            continue;
        }
        radiance *= TraceShadowPunctual(surface.position, spotLight.position, surface.normal,
                                        gLights.rayTracedShadowMask, kShadowMaskSpot, spotLight.sourceRadius);
        light += radiance * (facing / kPI);
    }

    if (useProbes)
    {
        float giWeight;
        float3 indirect = DDGIGetIrradiance(surface.position, surface.normal, -direction, gSampler, giWeight);
        light += lerp(RayMissRadiance(surface.normal), indirect, giWeight);
    }
    else
    {
        light += RayMissRadiance(surface.normal);
    }

    return surface.albedo * light + surface.emission;
}

#endif // RAY_HIT_LIGHTING_HLSLI
