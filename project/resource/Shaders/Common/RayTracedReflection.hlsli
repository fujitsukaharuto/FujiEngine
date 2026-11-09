#ifndef RAYTRACED_REFLECTION_HLSLI
#define RAYTRACED_REFLECTION_HLSLI

// 鏡面の環境光を、インラインレイトレで遮蔽しながら引く。
// TLAS と RayQuery は影と共有するので RayTracedShadow.hlsli を使う
#include "RayTracedShadow.hlsli"

// 環境キューブマップ。space0 は gTextures[]、space2 は TLAS が使用中なので space1。
// シェーダごとに宣言すると space がずれるので、必ずこのヘッダを include すること
TextureCube<float4> gEnvironment : register(t0, space1);

// AllLights::reflectionMode の値。C++ 側 (LightManager.h の RayTracedReflectionMode) と同じ値であること
static const uint kReflectionModeOff = 0;       // 遮蔽を見ない。どの向きにも空が映る
static const uint kReflectionModeOcclusion = 1; // 物に遮られた向きには空を映さない
static const uint kReflectionModeObjects = 2;   // 遮った物の色を映す

// 当たった物の見た目と形 (gRayInstances / gRayVertices / gRayIndices)
#include "RayInstance.hlsli"

struct ReflectionHit
{
    bool isHit;
    // 以下は kReflectionModeObjects で当たったときだけ入る
    float3 color;
    float3 emissive;
    uint textureIndex;
    uint emissiveTextureIndex;
    float3 position; // 当たった点
    float3 toOrigin; // 当たった点からレイの出どころへ向かう向き
    bool hasUV;
    float2 uv;
    bool hasNormal;
    float3 normal; // レイの来た側を向くように揃えてある
    float hitDistance; // 当たった距離
    float opacity;  // ガラスなら1未満
};

/// <summary>InstanceMask で絞ってレイを飛ばし、いちばん手前に当たった物を返す</summary>
ReflectionHit TraceReflectionMasked(float3 origin, float3 R, float maxDistance, uint mask)
{
    ReflectionHit hit = (ReflectionHit) 0;
    hit.toOrigin = -R;
    hit.normal = -R;
    hit.opacity = 1.0f;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = R;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;

    // 映すのはいちばん手前の物なので、最初の1つで打ち切れない
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, mask, ray);
    q.Proceed();

    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        return hit;
    }

    RayInstance instance = gRayInstances[q.CommittedInstanceID() + q.CommittedGeometryIndex()];
    hit.isHit = true;
    hit.color = instance.color;
    hit.emissive = instance.emissive;
    hit.textureIndex = instance.textureIndex;
    hit.emissiveTextureIndex = instance.emissiveTextureIndex;
    hit.position = ray.Origin + R * q.CommittedRayT();
    hit.hitDistance = q.CommittedRayT();
    hit.opacity = saturate(instance.opacity);

    if ((instance.flags & kRayInstanceHasGeometry) != 0)
    {
        // 当たった三角形の3頂点を引いて、当たった位置の値に補間する
        uint first = q.CommittedPrimitiveIndex() * 3;
        Buffer<uint> indices = gRayIndices[NonUniformResourceIndex(instance.indexBuffer)];
        StructuredBuffer<RayVertex> vertices = gRayVertices[NonUniformResourceIndex(instance.vertexBuffer)];
        RayVertex v0 = vertices[indices[first + 0]];
        RayVertex v1 = vertices[indices[first + 1]];
        RayVertex v2 = vertices[indices[first + 2]];

        float2 barycentrics = q.CommittedTriangleBarycentrics();
        float3 weight = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

        float2 uv = v0.texcoord * weight.x + v1.texcoord * weight.y + v2.texcoord * weight.z;
        // Material::SetUVScale と同じ順(ずらしてから拡縮)
        hit.uv = (uv + instance.uvOffset) * instance.uvScale;
        hit.hasUV = true;

        if ((instance.flags & kRayInstanceHasNormal) != 0)
        {
            float3 objectNormal = v0.normal * weight.x + v1.normal * weight.y + v2.normal * weight.z;
            float3 worldNormal = mul((float3x3) q.CommittedObjectToWorld3x4(), objectNormal);
            if (dot(worldNormal, worldNormal) > 1e-8f)
            {
                worldNormal = normalize(worldNormal);
                // 裏から当たったら裏返す
                hit.normal = dot(worldNormal, R) > 0.0f ? -worldNormal : worldNormal;
                hit.hasNormal = true;
            }
        }
    }

    return hit;
}

/// <summary>反射方向へレイを飛ばし、何に当たったかを返す</summary>
/// <remarks>Objects 以外では当たったかどうかだけ調べる</remarks>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="R">反射方向(正規化済み)</param>
/// <param name="N">面の法線</param>
/// <param name="maxDistance">この距離までの物を見る</param>
/// <param name="mode">AllLights::reflectionMode</param>
ReflectionHit TraceReflectionHit(float3 worldPos, float3 R, float3 N, float maxDistance, uint mode)
{
    ReflectionHit hit;
    hit.isHit = false;
    hit.color = float3(0.0f, 0.0f, 0.0f);
    hit.emissive = float3(0.0f, 0.0f, 0.0f);
    hit.textureIndex = 0;
    hit.emissiveTextureIndex = 0;
    hit.position = worldPos;
    hit.toOrigin = -R;
    hit.hasUV = false;
    hit.uv = float2(0.5f, 0.5f);
    hit.hasNormal = false;
    hit.normal = -R;
    hit.hitDistance = 0.0f;
    hit.opacity = 1.0f;

    if (mode == kReflectionModeOff || maxDistance <= 0.0f)
    {
        return hit;
    }

    if (mode != kReflectionModeObjects)
    {
        hit.isHit = TraceShadowRay(worldPos, R, N, maxDistance) == 0.0f;
        return hit;
    }

    // 不透明な物だけ
    return TraceReflectionMasked(worldPos + N * kShadowRayBias, R, maxDistance, kRayMaskOpaque);
}

/// <summary>反射方向にあるいちばん手前のガラス。不透明な物より奥は見ない</summary>
/// <param name="maxDistance">不透明な物に当たったならその距離</param>
ReflectionHit TraceReflectionGlass(float3 worldPos, float3 R, float3 N, float maxDistance)
{
    return TraceReflectionMasked(worldPos + N * kShadowRayBias, R, maxDistance, kRayMaskTranslucent);
}

/// <summary>当たった物の、反射に映す色を返す</summary>
/// <remarks>当てるのはアンビエントと0番の平行光源だけ。法線の無い物は半分だけ光を向いた面として扱う</remarks>
/// <param name="textureColor">当たった点のテクスチャの色</param>
/// <param name="L">平行光源へ向かう単位ベクトル</param>
/// <param name="sunRadiance">平行光源の色と強さ。無ければ0</param>
/// <param name="shadowMask">AllLights::rayTracedShadowMask</param>
float3 ShadeReflectionHit(ReflectionHit hit, float3 textureColor, float3 emissiveColor, float3 skyColor, float3 groundColor,
                          float ambientIntensity, float3 L, float3 sunRadiance, uint shadowMask)
{
    float3 albedo = hit.color * textureColor;

    float skyAmount = 0.5f;
    float facing = 0.5f;
    if (hit.hasNormal)
    {
        skyAmount = hit.normal.y * 0.5f + 0.5f;
        facing = saturate(dot(hit.normal, L));
    }
    float3 light = lerp(groundColor, skyColor, skyAmount) * ambientIntensity;

    if (facing > 0.0f && any(sunRadiance > 0.0f))
    {
        // 映った先でも影は落ちる。原点を浮かせる向きは、法線が無ければレイの来た側
        float shadow = TraceShadowDirectional(hit.position, L, hit.normal, shadowMask);
        light += sunRadiance * (facing * shadow / kSamplingPI);
    }

    return albedo * light + hit.emissive * emissiveColor;
}

#endif // RAYTRACED_REFLECTION_HLSLI
