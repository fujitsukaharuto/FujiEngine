#ifndef CLUSTERED_LIGHTS_HLSLI
#define CLUSTERED_LIGHTS_HLSLI

// 影を落とさないライト (クラスタ化ライティング / Forward+)。画素は自分のクラスタの一覧だけを回す

// ClusteredLightPass.h と同じ値であること
static const uint kClusterCountX = 16;
static const uint kClusterCountY = 9;
static const uint kClusterCountZ = 24;
static const uint kMaxLightsPerCluster = 128;

// ClusteredLight::type の値。LightManager.h の ClusteredLightType と同じ値であること
static const uint kClusteredLightPoint = 0;
static const uint kClusteredLightSpot = 1;
static const uint kClusteredLightTube = 2; // 棒。direction が軸 × 長さの半分
static const uint kClusteredLightRect = 3; // 長方形。direction が横 × 幅の半分、RectAxisV が縦 × 高さの半分

// LightManager.h の ClusteredLightData と並びが一致していること
struct ClusteredLight
{
    float3 position;
    float range;            // 光が届く距離
    float3 color;           // 色 × 強さ
    float decay;            // 距離による減り方
    float3 direction;       // スポットライトの照らす向き
    float cosAngle;         // スポットライトの外側の角度の cos
    float cosFalloffStart;  // 内側の角度の cos
    uint type;
    float2 pad;             // 長方形: x は縦の軸の z、y が 0.5 より大きければ両面
};

// ClusteredLightPass.h の ClusterParams と並びが一致していること
struct ClusterParams
{
    float4x4 view;
    float2 projectionScale; // 射影行列の [0][0] と [1][1]
    float clusterNear;      // いちばん手前の段の距離
    float sliceScale;       // 段の番号 = log(距離 / clusterNear) × これ
    float2 screenSize;      // 描画の解像度
    uint lightCount;        // 0 ならライトが無い
    float clusterFar;
};

/// <summary>画素の位置とカメラからの奥行きから、クラスタの番号</summary>
uint ClusterIndex(float2 pixel, float viewZ, ClusterParams params)
{
    uint2 tile = min(uint2(pixel / params.screenSize * float2(kClusterCountX, kClusterCountY)),
                     uint2(kClusterCountX - 1, kClusterCountY - 1));
    float slice = log(max(viewZ, params.clusterNear) / params.clusterNear) * params.sliceScale;
    uint z = min(uint(max(slice, 0.0f)), kClusterCountZ - 1);
    return tile.x + kClusterCountX * (tile.y + kClusterCountY * z);
}

/// <summary>距離と向きによる明るさの割合</summary>
float ClusteredLightAttenuation(ClusteredLight light, float distance, float3 L)
{
    float attenuation = pow(saturate(1.0f - distance / light.range), light.decay);
    if (light.type == kClusteredLightSpot)
    {
        float cosAngle = dot(-L, light.direction);
        attenuation *= saturate((cosAngle - light.cosAngle) / (light.cosFalloffStart - light.cosAngle));
    }
    return attenuation;
}

/// <summary>長方形の縦の軸 (向き × 高さの半分)</summary>
float3 ClusteredLightRectAxisV(ClusteredLight light)
{
    return float3(light.cosAngle, light.cosFalloffStart, light.pad.x);
}

/// <summary>形のある光源の中心から端までの長さ</summary>
float ClusteredLightExtent(ClusteredLight light)
{
    if (light.type == kClusteredLightTube)
    {
        return length(light.direction);
    }
    if (light.type == kClusteredLightRect)
    {
        return length(light.direction + ClusteredLightRectAxisV(light));
    }
    return 0.0f;
}

/// <summary>光源の上で点 P にいちばん近い点</summary>
float3 ClusteredLightNearestPoint(ClusteredLight light, float3 P)
{
    if (light.type == kClusteredLightTube)
    {
        float3 axis = light.direction;
        float t = clamp(dot(P - light.position, axis) / max(dot(axis, axis), 1e-8f), -1.0f, 1.0f);
        return light.position + axis * t;
    }
    if (light.type == kClusteredLightRect)
    {
        float3 u = light.direction;
        float3 v = ClusteredLightRectAxisV(light);
        float3 local = P - light.position;
        float su = clamp(dot(local, u) / max(dot(u, u), 1e-8f), -1.0f, 1.0f);
        float sv = clamp(dot(local, v) / max(dot(v, v), 1e-8f), -1.0f, 1.0f);
        return light.position + u * su + v * sv;
    }
    return light.position;
}

/// <summary>光源の上で反射の向き R にいちばん近い点 (Karis 2013 の代表点)</summary>
float3 ClusteredLightSpecularPoint(ClusteredLight light, float3 P, float3 R)
{
    if (light.type == kClusteredLightTube)
    {
        float3 l0 = light.position - light.direction - P;
        float3 ld = light.direction * 2.0f;
        float rDotL0 = dot(R, l0);
        float rDotLd = dot(R, ld);
        float t = (rDotL0 * rDotLd - dot(l0, ld)) / max(dot(ld, ld) - rDotLd * rDotLd, 1e-6f);
        return P + l0 + ld * saturate(t);
    }
    if (light.type == kClusteredLightRect)
    {
        // 反射の向きと面の交点を長方形の中へ寄せる
        float3 u = light.direction;
        float3 v = ClusteredLightRectAxisV(light);
        float3 n = cross(u, v);
        float denominator = dot(R, n);
        if (abs(denominator) > 1e-6f)
        {
            float t = dot(light.position - P, n) / denominator;
            if (t > 0.0f)
            {
                return ClusteredLightNearestPoint(light, P + R * t);
            }
        }
        return ClusteredLightNearestPoint(light, P);
    }
    return light.position;
}

/// <summary>形のある光源の面の向きによる明るさ。長方形は表 (両面なら裏も) だけ</summary>
/// <param name="toSurface">光源の上の点から照らす点への向き (正規化済み)</param>
float ClusteredLightEmitterFacing(ClusteredLight light, float3 toSurface)
{
    if (light.type != kClusteredLightRect)
    {
        return 1.0f;
    }
    float3 n = normalize(cross(light.direction, ClusteredLightRectAxisV(light)));
    float facing = dot(n, toSurface);
    if (light.pad.y > 0.5f)
    {
        facing = abs(facing);
    }
    return saturate(facing);
}

/// <summary>形のある光源のハイライトの明るさの割合 (Karis 2013 のエネルギーの正規化)</summary>
/// <param name="roughness">見た目の粗さ (2乗が GGX の a)</param>
/// <param name="distance">代表点までの距離</param>
float ClusteredLightSpecularNormalization(ClusteredLight light, float roughness, float distance)
{
    if (light.type != kClusteredLightTube && light.type != kClusteredLightRect)
    {
        return 1.0f;
    }
    float a = max(roughness * roughness, 1e-4f);
    float invDistance = 1.0f / max(distance, 1e-3f);
    float normalization = a / saturate(a + length(light.direction) * invDistance);
    if (light.type == kClusteredLightRect)
    {
        normalization *= a / saturate(a + length(ClusteredLightRectAxisV(light)) * invDistance);
    }
    return normalization;
}

/// <summary>点 P がどれだけ照らされるか</summary>
/// <param name="L">P から光の来る点への向き</param>
/// <param name="lightDistance">光の来る点までの距離</param>
float ClusteredLightShapeAttenuation(ClusteredLight light, float3 P, out float3 L, out float lightDistance)
{
    float3 lightPoint = ClusteredLightNearestPoint(light, P);
    float3 toLight = lightPoint - P;
    lightDistance = length(toLight);
    L = lightDistance > 0.0f ? toLight / lightDistance : float3(0.0f, 1.0f, 0.0f);
    if (lightDistance >= light.range)
    {
        return 0.0f;
    }
    return ClusteredLightAttenuation(light, lightDistance, L) * ClusteredLightEmitterFacing(light, -L);
}

/// <summary>ワールドの位置が今の画面のどのクラスタに入るか。画面の外やカメラの後ろなら false</summary>
bool ClusterIndexAtWorld(float3 worldPos, ClusterParams params, out uint cluster)
{
    cluster = 0;
    float3 viewPos = mul(float4(worldPos, 1.0f), params.view).xyz;
    if (viewPos.z <= 1e-3f)
    {
        return false;
    }
    float2 ndc = viewPos.xy * params.projectionScale / viewPos.z;
    if (any(abs(ndc) > 1.0f))
    {
        return false;
    }
    float2 pixel = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f) * params.screenSize;
    cluster = ClusterIndex(pixel, viewPos.z, params);
    return true;
}

/// <summary>影なしのライトが拡散面に届ける光の和 (色 × 減衰 × N・L)。π では割っていない</summary>
/// <remarks>今の画面のクラスタを引く。画面の外の点は 0</remarks>
float3 ClusteredIrradianceAt(float3 position, float3 normal, ClusterParams params, StructuredBuffer<ClusteredLight> lights,
                             StructuredBuffer<uint> counts, StructuredBuffer<uint> indices)
{
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    uint cluster;
    if (params.lightCount == 0 || !ClusterIndexAtWorld(position, params, cluster))
    {
        return irradiance;
    }
    uint count = counts[cluster];
    for (uint c = 0; c < count; c++)
    {
        ClusteredLight light = lights[indices[cluster * kMaxLightsPerCluster + c]];
        // 形のある光源はいちばん近い点から
        float3 L;
        float distance;
        float attenuation = ClusteredLightShapeAttenuation(light, position, L, distance);
        float facing = saturate(dot(normal, L));
        if (attenuation <= 0.0f || facing <= 0.0f)
        {
            continue;
        }
        irradiance += light.color * (attenuation * facing);
    }
    return irradiance;
}

#endif // CLUSTERED_LIGHTS_HLSLI
