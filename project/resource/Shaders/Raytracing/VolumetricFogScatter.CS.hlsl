// 抜きのある物 (葉など) の影も抜けた形にする
#define RAY_CUTOUT_SHADOW
#include "../Object/Object3d.hlsli"
#include "../Common/RayTracedShadow.hlsli"
#include "../Common/IBL.hlsli"
#include "../Common/ClusteredLights.hlsli"
#include "../Common/Atmosphere.hlsli"
#include "../Common/VolumetricFog.hlsli"

// ボリュメトリックフォグの箱1つぶん (UE の MaterialSetup と LightScattering)。
// 箱の中のずらした1点で霧の濃さと受ける光を求め、前のフレームの値と混ぜて書く。
// rgb = 1mあたりにカメラの向きへ散る光、a = 消散 (1mあたり)

ConstantBuffer<AllLights> gLights : register(b0);

// 影なしのライト (ClusteredLightPass)
ConstantBuffer<ClusterParams> gClusterParams : register(b3);
StructuredBuffer<ClusteredLight> gClusteredLights : register(t0, space10);
StructuredBuffer<uint> gClusterCounts : register(t1, space10);
StructuredBuffer<uint> gClusterIndices : register(t2, space10);

Texture3D<float4> gVolumetricFogHistory : register(t0, space15);
RWTexture3D<float4> gVolumetricFogScatterOut : register(u0);
SamplerState gSampler : register(s0);

/// <summary>Henyey-Greenstein の位相関数</summary>
/// <param name="cosTheta">光の進む向きと、散ってカメラへ向かう向きのなす角の cos</param>
float HenyeyGreenstein(float cosTheta, float g)
{
    float denominator = max(1.0f + g * g - 2.0f * g * cosTheta, 1e-4f);
    return (1.0f - g * g) / (4.0f * kPI * denominator * sqrt(denominator));
}

/// <summary>箱の中の点のワールド座標</summary>
/// <param name="cell">箱の番号 + 箱の中の位置 (0〜1)</param>
/// <param name="viewDepth">その点の奥行き</param>
float3 CellPosition(float3 cell, out float viewDepth)
{
    float2 screenUV = cell.xy / float2(gVolumetricFog.gridSize.xy) / gVolumetricFog.screenToGridUV;
    float2 ndc = float2(screenUV.x * 2.0f - 1.0f, 1.0f - screenUV.y * 2.0f);
    float4 nearPoint = mul(float4(ndc, 0.0f, 1.0f), gVolumetricFog.invViewProj);
    float4 farPoint = mul(float4(ndc, 1.0f, 1.0f), gVolumetricFog.invViewProj);
    float3 direction = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);
    viewDepth = VolumetricFogDepthFromSlice(cell.z);
    return gVolumetricFog.cameraPosition + direction * (viewDepth / max(dot(direction, gVolumetricFog.cameraForward), 1e-4f));
}

/// <summary>その点が光源から受けて、カメラの向きへ散らす光 (散乱の係数は掛けていない)</summary>
/// <param name="direction">カメラからその点への向き</param>
float3 InscatteredLight(float3 position, float3 direction, float viewDepth, float2 pixel)
{
    float g = gVolumetricFog.phaseG;
    float3 lighting = float3(0.0f, 0.0f, 0.0f);

    // 平行光源。影はレイで見る
    if (gVolumetricFog.sunScale > 0.0f)
    {
        for (int i = 0; i < gLights.numDirectionalLights; i++)
        {
            DirectionalLight light = gLights.directionalLights[i];
            float3 radiance = light.color.rgb * light.intensity;
            if (!any(radiance > 0.0f))
            {
                continue;
            }
            float3 L = normalize(-light.direction);
            float shadow = 1.0f;
            if ((gLights.rayTracedShadowMask & kShadowMaskDirectional) != 0)
            {
                shadow = TraceShadowRayBiased(position, L, L, kShadowRayMaxDistance, 0.0f);
            }
            if (i == 0 && gLights.fog.skyMode == kSkyModeAtmosphere)
            {
                // 雲が太陽を遮る
                shadow *= CloudShadow(position, L, gLights.fog.cloudCoverage, gLights.fog.cloudTime, gLights.fog.cloudShadow);
            }
            if (shadow > 0.0f)
            {
                lighting += radiance * (shadow * HenyeyGreenstein(dot(direction, L), g) * gVolumetricFog.sunScale);
            }
        }
    }

    // 空からの光。UE と同じく、カメラへ向いた面に届く拡散光で代える (遮られない)
    if (gVolumetricFog.skyLightScale > 0.0f)
    {
        lighting += GetAmbientDiffuse(-direction, gLights.ambientMode, gLights.ambientSkyColor, gLights.ambientGroundColor,
                                      gLights.ambientIntensity, gLights.iblAmbientScale, gLights.environmentTint, gSampler)
                    * gVolumetricFog.skyLightScale;
    }

    if (gVolumetricFog.localLightScale <= 0.0f)
    {
        return lighting;
    }
    float3 localLighting = float3(0.0f, 0.0f, 0.0f);

    // 影ありの点光源とスポット
    for (int j = 0; j < gLights.numPointLights; j++)
    {
        PointLight light = gLights.pointLights[j];
        float3 radiance = light.color.rgb * light.intensity * PointLightAttenuation(light, position);
        if (!any(radiance > 0.0f))
        {
            continue;
        }
        float3 L = normalize(light.position - position);
        radiance *= TraceShadowPunctual(position, light.position, L, gLights.rayTracedShadowMask, kShadowMaskPoint, light.sourceRadius);
        localLighting += radiance * HenyeyGreenstein(dot(direction, L), g);
    }
    for (int k = 0; k < gLights.numSpotLights; k++)
    {
        SpotLight light = gLights.spotLights[k];
        float3 lightDirOnPoint = normalize(position - light.position);
        float3 radiance = light.color.rgb * light.intensity * SpotLightAttenuation(light, position, lightDirOnPoint);
        if (!any(radiance > 0.0f))
        {
            continue;
        }
        float3 L = -lightDirOnPoint;
        radiance *= TraceShadowPunctual(position, light.position, L, gLights.rayTracedShadowMask, kShadowMaskSpot, light.sourceRadius);
        localLighting += radiance * HenyeyGreenstein(dot(direction, L), g);
    }

    // 影なしのライト。その点のクラスタに届くものだけ。形のある光源はいちばん近い点から
    if (gClusterParams.lightCount > 0)
    {
        uint cluster = ClusterIndex(pixel, viewDepth, gClusterParams);
        uint count = gClusterCounts[cluster];
        for (uint c = 0; c < count; c++)
        {
            ClusteredLight light = gClusteredLights[gClusterIndices[cluster * kMaxLightsPerCluster + c]];
            float3 L;
            float distance;
            float attenuation = ClusteredLightShapeAttenuation(light, position, L, distance);
            if (attenuation <= 0.0f || distance <= 0.0f)
            {
                continue;
            }
            localLighting += light.color * (attenuation * HenyeyGreenstein(dot(direction, L), g));
        }
    }

    return lighting + localLighting * gVolumetricFog.localLightScale;
}

/// <summary>前のフレームの同じ場所の値。画面の外や手前なら false</summary>
bool SampleHistory(float3 position, out float4 history)
{
    history = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 clip = mul(float4(position, 1.0f), gVolumetricFog.prevViewProj);
    if (clip.w <= 1e-4f)
    {
        return false;
    }
    float2 ndc = clip.xy / clip.w;
    float2 screenUV = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
    // 透視投影の w はビュー空間の z
    float3 size = float3(gVolumetricFog.gridSize);
    float3 uvw = float3(screenUV * gVolumetricFog.screenToGridUV, VolumetricFogSliceFromDepth(clip.w) / size.z);
    if (any(uvw < 0.0f) || any(uvw > 1.0f))
    {
        return false;
    }
    float3 halfTexel = 0.5f / size;
    history = gVolumetricFogHistory.SampleLevel(gSampler, clamp(uvw, halfTexel, 1.0f - halfTexel), 0.0f);
    return true;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(dispatchID >= gVolumetricFog.gridSize))
    {
        return;
    }

    // 箱の中で毎フレームずらした1点
    float viewDepth;
    float3 position = CellPosition(float3(dispatchID) + gVolumetricFog.jitter, viewDepth);
    float3 toPoint = position - gVolumetricFog.cameraPosition;
    float distance = length(toPoint);
    float3 direction = toPoint / max(distance, 1e-6f);

    // 高さの霧。式の霧と同じく、始まる距離より手前には無い。散乱は濃さ × アルベド (UE と同じく、消散は灰色のまま)
    float extinction = 0.0f;
    float3 scattering = float3(0.0f, 0.0f, 0.0f);
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    if (distance >= gLights.fog.startDistance)
    {
        extinction = FogDensityAt(gLights.fog, position.y) * gVolumetricFog.extinctionScale;
        scattering = gVolumetricFog.albedo * extinction;
        emissive = gVolumetricFog.emissive;
    }
    // 局所の霧は始まる距離に関係なく足す
    float3 localScattering;
    float3 localEmissive;
    extinction += LocalFogVolumesAt(position, localScattering, localEmissive);
    scattering += localScattering;
    emissive += localEmissive;

    float3 inScatter = emissive;
    if (any(scattering > 0.0f))
    {
        float2 screenUV = (float2(dispatchID.xy) + gVolumetricFog.jitter.xy) / float2(gVolumetricFog.gridSize.xy) / gVolumetricFog.screenToGridUV;
        float2 pixel = screenUV / gVolumetricFog.invScreenSize;
        inScatter += scattering * InscatteredLight(position, direction, viewDepth, pixel);
    }
    float4 current = float4(inScatter, extinction);

    // 前のフレームと混ぜる。履歴は箱の中心の値として持つので、中心を前のカメラへ写す
    if (gVolumetricFog.historyWeight > 0.0f)
    {
        float centerDepth;
        float3 center = CellPosition(float3(dispatchID) + 0.5f, centerDepth);
        float4 history;
        if (SampleHistory(center, history))
        {
            current = lerp(current, history, gVolumetricFog.historyWeight);
        }
    }

    gVolumetricFogScatterOut[dispatchID] = current;
}
