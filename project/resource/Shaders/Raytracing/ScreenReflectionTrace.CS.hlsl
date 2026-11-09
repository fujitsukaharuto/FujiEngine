#include "../Object/Object3d.hlsli"

// 映り込みのレイを画素ごとに鏡の向きへ1本飛ばす。何にも当たらなければ空は前方描画が引く

Texture2D<float4> gTextures[] : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b0);
#include "../Common/RayHitLighting.hlsli"
#include "../Common/ScreenReflectionCommon.hlsli"

RWTexture2D<float4> gReflectionTraceOut : register(u0); // rgb=当たった物の明るさ, a=距離。空なら -1、飛ばしていなければ 0
Texture2D<float4> gReflectionSceneColor : register(t6, space8); // 前のフレームの、不透明の物を描き終えた画面

// reflectAll で付けた映り込みは、はっきり見える所だけ飛ばす (Object3d.PS の visibleWeight と同じ)
static const float kImplicitVisibleWeight = 0.1f;
// 2回目の跳ね返りを飛ばす粗さの上限
static const float kBounceMaxRoughness = 0.4f;
// 画面の端からこの割合の内側で画面の色へ溶かす
static const float kSceneColorEdgeFade = 0.05f;
// これより霧が濃い所は画面の色を使わない
static const float kSceneColorMaxFog = 0.9f;

/// <summary>画面の上の UV。カメラの後ろなら範囲外の値を返す</summary>
float2 ProjectToUV(float3 position, float4x4 projection)
{
    float4 clip = mul(float4(position, 1.0f), projection);
    if (clip.w <= 1e-4f)
    {
        return float2(-1.0f, -1.0f);
    }
    float2 ndc = clip.xy / clip.w;
    return float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
}

/// <summary>画面の端からの近さで、画面の色をどれだけ使うか</summary>
float EdgeWeight(float2 uv)
{
    float edge = min(min(uv.x, 1.0f - uv.x), min(uv.y, 1.0f - uv.y));
    return saturate(edge / kSceneColorEdgeFade);
}

/// <summary>レイが当たった所が今の画面に写っていれば、写した画面からその色を引く (SSR)</summary>
/// <remarks>前のフレームの画面なので動く物は少し遅れる</remarks>
/// <returns>画面の色を使う割合。0なら使えない</returns>
float SampleSceneColor(float3 hitPosition, out float3 color)
{
    color = float3(0.0f, 0.0f, 0.0f);

    // 手前の物に隠れていないか
    float2 uv = ProjectToUV(hitPosition, viewProj);
    float weight = EdgeWeight(uv);
    if (weight <= 0.0f)
    {
        return 0.0f;
    }
    int2 pixel = min(int2(uv * float2(screenSize)), int2(screenSize) - 1);
    float depth = gGBufferDepth.Load(int3(pixel, 0)).r;
    if (depth >= 1.0f)
    {
        return 0.0f;
    }
    float hitDistance = distance(hitPosition, cameraPos);
    float visibleDistance = distance(ReconstructWorldPos(uint2(pixel), screenSize, depth, invViewProj), cameraPos);
    if (abs(visibleDistance - hitDistance) > hitDistance * 0.02f + 0.05f)
    {
        return 0.0f;
    }
    // 映り込みを持つ面は映し合って明るさが積み上がるので、レイの結果で光らせる
    float hitRoughness;
    bool hitIsExplicit;
    if (DecodeReflectionRoughness(gGBufferNormal.Load(int3(pixel, 0)).a, hitRoughness, hitIsExplicit))
    {
        return 0.0f;
    }

    float2 sceneUV = ProjectToUV(hitPosition, sceneColorViewProj);
    weight = min(weight, EdgeWeight(sceneUV));
    if (weight <= 0.0f)
    {
        return 0.0f;
    }
    color = gReflectionSceneColor.SampleLevel(gSampler, sceneUV, 0.0f).rgb;

    // 前方描画が霧を掛け直すので外しておく
    float3 toHit = hitPosition - cameraPos;
    float fogAmount = FogAmount(gLights.fog, cameraPos, hitPosition);
    if (fogAmount > 0.0f)
    {
        if (fogAmount > kSceneColorMaxFog)
        {
            return 0.0f;
        }
        float3 fogColor = FogColor(gLights.fog, toHit);
        color = (color - fogColor * fogAmount) / (1.0f - fogAmount);
    }
    color = max(RemoveAerialPerspective(gLights.fog, color, toHit), 0.0f);
    return weight;
}

/// <summary>反射の向きの、不透明な物より手前にあるガラスを重ねる</summary>
/// <param name="radiance">不透明な物の明るさ。当たっていなければ空の明るさ</param>
/// <param name="maxDistance">不透明な物に当たった距離 (当たっていなければレイの長さ)</param>
/// <returns>ガラスに当たったか</returns>
bool BlendReflectionGlass(float3 origin, float3 direction, float maxDistance, bool useProbes, inout float3 radiance, out float glassDistance)
{
    glassDistance = maxDistance;
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskTranslucent, ray);
    q.Proceed();
    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        return false;
    }
    glassDistance = max(q.CommittedRayT(), 1e-4f);
    uint instanceIndex = q.CommittedInstanceID() + q.CommittedGeometryIndex();
    RayHitSurface glass = LoadRayHitSurface(instanceIndex, q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                            q.CommittedObjectToWorld3x4(), origin, direction, glassDistance);
    radiance = lerp(radiance, ShadeRayHit(glass, direction, useProbes), saturate(gRayInstances[instanceIndex].opacity));
    return true;
}

/// <summary>当たった面が映り込む物なら、そこに映った物を足す</summary>
/// <param name="radiance">当たった面を ShadeRayHit で光らせた明るさ</param>
float3 AddReflectionBounce(RayHitSurface surface, float3 direction, float3 radiance, bool useProbes)
{
    float bounceWeight = saturate((kBounceMaxRoughness - surface.roughness) / 0.1f);
    if (bounceWeight <= 0.0f)
    {
        return radiance;
    }
    float3 f0 = lerp(kDielectricF0, surface.albedo, surface.metallic);
    float3 fresnel = F_Schlick(f0, saturate(dot(surface.normal, -direction)));
    float3 bounceDirection = reflect(direction, surface.normal);

    RayDesc ray;
    ray.Origin = surface.position + surface.normal * kShadowRayBias;
    ray.Direction = bounceDirection;
    ray.TMin = 0.0f;
    ray.TMax = gLights.reflectionMaxDistance;
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
    q.Proceed();

    float3 bounceRadiance;
    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        // 空。前方描画が足せないのでここで引く
        bounceRadiance = (gLights.hasEnvironment != 0)
            ? gPrefilteredEnv.SampleLevel(gSampler, bounceDirection, surface.roughness * kPrefilteredMaxMip).rgb * gLights.environmentTint
            : HemisphereAmbient(bounceDirection, gLights.ambientSkyColor, gLights.ambientGroundColor, gLights.ambientIntensity);
    }
    else if (q.CommittedTriangleFrontFace())
    {
        RayHitSurface bounce = LoadRayHitSurface(q.CommittedInstanceID() + q.CommittedGeometryIndex(),
                                                 q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                                 q.CommittedObjectToWorld3x4(), ray.Origin, bounceDirection,
                                                 max(q.CommittedRayT(), 1e-4f));
        bounceRadiance = ShadeRayHit(bounce, bounceDirection, useProbes);
    }
    else
    {
        bounceRadiance = float3(0.0f, 0.0f, 0.0f);
    }
    // 金属は映った物が拡散のぶんを置き換える
    return lerp(radiance, radiance * (1.0f - surface.metallic) + fresnel * bounceRadiance, bounceWeight);
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
    float roughness;
    bool isExplicit;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth) ||
        !DecodeReflectionRoughness(gGBufferNormal.Load(int3(pixel, 0)).a, roughness, isExplicit))
    {
        gReflectionTraceOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float3 position = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
    float3 toEye = normalize(cameraPos - position);
    if (!isExplicit)
    {
        // 映り込みがはっきり見える角度か
        float fresnel = F_Schlick(kDielectricF0, saturate(dot(normal, toEye))).r;
        if (fresnel * (1.0f - roughness) < kImplicitVisibleWeight)
        {
            gReflectionTraceOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
    }
    float3 direction = reflect(-toEye, normal);

    RayDesc ray;
    ray.Origin = position + normal * ReconstructedRayBias(distance(position, cameraPos));
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = gLights.reflectionMaxDistance;

    // 不透明な物だけ
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskOpaque, ray);
    q.Proceed();

    bool useGlassProbes = (gLights.renderFlags & kRenderFlagGI) != 0;
    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        // ガラス越しの空はここで混ぜる
        float3 skyRadiance = (gLights.hasEnvironment != 0)
            ? gPrefilteredEnv.SampleLevel(gSampler, direction, roughness * kPrefilteredMaxMip).rgb * gLights.environmentTint
            : HemisphereAmbient(direction, gLights.ambientSkyColor, gLights.ambientGroundColor, gLights.ambientIntensity);
        float glassDistance;
        if (BlendReflectionGlass(ray.Origin, direction, gLights.reflectionMaxDistance, useGlassProbes, skyRadiance, glassDistance))
        {
            gReflectionTraceOut[pixel] = float4(skyRadiance, glassDistance);
            return;
        }
        gReflectionTraceOut[pixel] = float4(0.0f, 0.0f, 0.0f, -1.0f);
        return;
    }
    float hitDistance = max(q.CommittedRayT(), 1e-4f);
    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    // 裏面は暗いまま
    if (q.CommittedTriangleFrontFace())
    {
        float3 sceneColor = float3(0.0f, 0.0f, 0.0f);
        float sceneWeight = 0.0f;
        if (useSceneColor != 0)
        {
            sceneWeight = SampleSceneColor(ray.Origin + direction * hitDistance, sceneColor);
        }
        // 画面の色だけで済む所は当たった面を光らせず、2回目も飛ばさない
        if (sceneWeight < 1.0f)
        {
            RayHitSurface surface = LoadRayHitSurface(q.CommittedInstanceID() + q.CommittedGeometryIndex(),
                                                      q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                                      q.CommittedObjectToWorld3x4(), ray.Origin, direction, hitDistance);
            bool useProbes = (gLights.renderFlags & kRenderFlagGI) != 0;
            radiance = ShadeRayHit(surface, direction, useProbes);
            if ((gLights.renderFlags & kRenderFlagReflectionBounce) != 0)
            {
                radiance = AddReflectionBounce(surface, direction, radiance, useProbes);
            }
        }
        radiance = lerp(radiance, sceneColor, sceneWeight);
    }
    float glassDistance;
    BlendReflectionGlass(ray.Origin, direction, hitDistance, useGlassProbes, radiance, glassDistance);
    gReflectionTraceOut[pixel] = float4(radiance, hitDistance);
}
