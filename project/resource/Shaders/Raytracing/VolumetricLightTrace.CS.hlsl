#include "../Object/Object3d.hlsli"
#include "../Common/PBR.hlsli"
#include "../Common/RayTracedShadow.hlsli"
#include "../Common/GBufferUtil.hlsli"
#include "../Common/VolumetricLightCommon.hlsli"
#include "../Common/ClusteredLights.hlsli"

// 光の筋。面までを等間隔に進み、各点から太陽へ影のレイを飛ばして散った光を足す
// 進み始めを 4x4 の模様でずらし、次のぼかしで均す。影なしのライトの光も影を見ずに拾う

ConstantBuffer<AllLights> gLights : register(b0);

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b1)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// gTextures[] が無いので space0 を使ってよい
Texture2D<float> gGBufferDepth : register(t0);
RWTexture2D<float4> gVolumetricTraceOut : register(u0);

// 影なしのライト (ClusteredLightPass)
ConstantBuffer<ClusterParams> gClusterParams : register(b3);
StructuredBuffer<ClusteredLight> gClusteredLights : register(t0, space10);
StructuredBuffer<uint> gClusterCounts : register(t1, space10);
StructuredBuffer<uint> gClusterIndices : register(t2, space10);

static const int kStepCount = 24;
// 4x4 のベイヤー配列
static const float kBayer4[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };

/// <summary>Henyey-Greenstein の位相関数</summary>
float HenyeyGreenstein(float cosTheta, float g)
{
    return (1.0f - g * g) / (4.0f * kPI * pow(max(1.0f + g * g - 2.0f * g * cosTheta, 1e-4f), 1.5f));
}

/// <summary>空気の1点が影なしのライトから受けてカメラへ散らす光 (濃さは掛けていない)</summary>
float3 LocalLightsInscatter(float3 position, float3 direction, float2 pixel)
{
    float viewZ = mul(float4(position, 1.0f), gClusterParams.view).z;
    uint cluster = ClusterIndex(pixel, viewZ, gClusterParams);
    uint count = gClusterCounts[cluster];
    float3 result = float3(0.0f, 0.0f, 0.0f);
    for (uint c = 0; c < count; c++)
    {
        ClusteredLight light = gClusteredLights[gClusterIndices[cluster * kMaxLightsPerCluster + c]];
        // 形のある光源はいちばん近い点から
        float3 L;
        float distance;
        float attenuation = ClusteredLightShapeAttenuation(light, position, L, distance);
        if (attenuation <= 0.0f || distance <= 0.0f)
        {
            continue;
        }
        result += light.color * attenuation * HenyeyGreenstein(dot(direction, L), anisotropy);
    }
    return result;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (any(pixel >= lowSize))
    {
        return;
    }

    // 4x4 画素の真ん中の深度で代表させる
    uint2 scale = screenSize / lowSize;
    uint2 full = min(pixel * scale + scale / 2, screenSize - 1);
    float depth = gGBufferDepth.Load(int3(full, 0));
    float viewDepth = VolumetricViewDepth(depth);

    bool useSun = sunScale > 0.0f && gLights.numDirectionalLights > 0;
    bool useLocal = localLightScale > 0.0f && gClusterParams.lightCount > 0;
    if (!useSun && !useLocal)
    {
        gVolumetricTraceOut[pixel] = float4(0.0f, 0.0f, 0.0f, viewDepth);
        return;
    }
    DirectionalLight sun = gLights.directionalLights[0];
    float3 L = normalize(-sun.direction);
    float3 radiance = sun.color.rgb * sun.intensity;

    float3 toPoint = ReconstructWorldPos(full, screenSize, depth, invViewProj) - cameraPos;
    float distance = length(toPoint);
    float3 direction = toPoint / max(distance, 1e-4f);
    float marchDistance = min(distance, maxDistance);

    float offset = (kBayer4[(pixel.y % 4) * 4 + pixel.x % 4] + 0.5f) / 16.0f;
    float stepLength = marchDistance / kStepCount;
    float lit = 0.0f;
    float3 localLit = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < kStepCount; ++i)
    {
        float t = (i + offset) * stepLength;
        float3 position = cameraPos + direction * t;
        // 遠くで散った光ほど空気に吸われる
        float transmittance = exp(-density * t);
        if (useSun)
        {
            lit += TraceShadowRayBiased(position, L, L, kShadowRayMaxDistance, 0.0f) * transmittance;
        }
        if (useLocal)
        {
            localLit += LocalLightsInscatter(position, direction, float2(full)) * transmittance;
        }
    }

    float3 sunLight = useSun ? radiance * (HenyeyGreenstein(dot(direction, L), anisotropy) * sunScale * lit) : float3(0.0f, 0.0f, 0.0f);
    gVolumetricTraceOut[pixel] = float4((sunLight + localLit * localLightScale) * (intensity * density * stepLength), viewDepth);
}
