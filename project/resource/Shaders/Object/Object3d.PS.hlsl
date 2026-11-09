// 抜きのある物の影を抜けた形にする (Common/RayTracedShadow.hlsli)
#define RAY_CUTOUT_SHADOW
#include "Object3d.hlsli"
#include "../Common/PBR.hlsli"
#include "../Common/SimpleLighting.hlsli"
#include "../Common/RayTracedShadow.hlsli"
#include "../Common/ScreenSpaceShadow.hlsli"
#include "../Common/RayTracedAO.hlsli"
#include "../Common/ScreenSpaceAO.hlsli"
#include "../Common/RayTracedReflection.hlsli"
#include "../Common/IBL.hlsli"
#include "../Common/DDGI.hlsli"
#include "../Common/BakedGI.hlsli"
#include "../Common/ReflectionProbe.hlsli"
#include "../Common/ScreenProbeGI.hlsli"
#include "../Common/ScreenReflection.hlsli"
#include "../Common/Refraction.hlsli"
#include "../Common/Atmosphere.hlsli"
#include "../Common/ClusteredLights.hlsli"
#include "../Common/Wetness.hlsli"

// 構造体(Material / 各種ライト / Camera)は Object3d.hlsli にまとめてある。
// ピッキングは Picking.PS.hlsl の別パス。UAVへ書くPSは隠れた画素でも最後まで実行される

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTextures[] : register(t0);

SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b2);
ConstantBuffer<Camera> gCamera : register(b3);

// 影なしのライト (ClusteredLightPass)。b4〜b6 と space9 は使用済み
ConstantBuffer<ClusterParams> gClusterParams : register(b7);
StructuredBuffer<ClusteredLight> gClusteredLights : register(t0, space10);
StructuredBuffer<uint> gClusterCounts : register(t1, space10);
StructuredBuffer<uint> gClusterIndices : register(t2, space10);

// これより粗い面は、形のある光源もいちばん近い点の光で済ませる
static const float kShapedLightRoughCutoff = 0.6f;
// 濡れた路面で屋根の下かを見る、真上へのレイの長さ
static const float kWetSkyCheckDistance = 100.0f;

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// <summary>半透明の物が落とす色の付いた影</summary>
float3 TranslucentShadow(float3 worldPos, float3 L, float3 N, float maxDistance)
{
    if ((gLights.renderFlags & kRenderFlagTranslucentShadow) == 0)
    {
        return float3(1.0f, 1.0f, 1.0f);
    }
    return TraceTranslucentTransmittance(worldPos, L, N, maxDistance);
}

float3 DirectBRDF(float3 N, float3 V, float3 L, float3 diffuseColor, float3 f0, float roughness, float3 energyCompensation)
{
    if (gLights.lightingMode == kLightingModePBR)
    {
        return BRDF(N, V, L, diffuseColor, f0, roughness, energyCompensation);
    }
    return SimpleBRDF(N, V, L, diffuseColor, f0, roughness);
}

/// <summary>直接光の拡散だけ</summary>
float3 DirectDiffuse(float3 N, float3 V, float3 L, float3 diffuseColor, float3 f0, float roughness)
{
    if (gLights.lightingMode == kLightingModePBR)
    {
        // 多重散乱の倍率を0にすると鏡面が消えて拡散だけが残る
        return BRDF(N, V, L, diffuseColor, f0, roughness, float3(0.0f, 0.0f, 0.0f));
    }
    return SimpleBRDF(N, V, L, diffuseColor, float3(0.0f, 0.0f, 0.0f), roughness);
}

/// <summary>直接光の鏡面だけ</summary>
float3 DirectSpecular(float3 N, float3 V, float3 L, float3 f0, float roughness, float3 energyCompensation)
{
    return DirectBRDF(N, V, L, float3(0.0f, 0.0f, 0.0f), f0, roughness, energyCompensation);
}

/// <summary>映り込みのレイ (Objects) が当たった物を光らせる</summary>
/// <param name="roughness">映している面の粗さ。粗いほどテクスチャをぼかす</param>
float3 ShadeForwardReflectionHit(ReflectionHit hit, float roughness)
{
    // 粗い面ほどぼかす。UVが引けなかった物はいちばん粗いミップ
    float hitLod = hit.hasUV ? 1.0f + roughness * 6.0f : 16.0f;
    float3 hitTexture = gTextures[NonUniformResourceIndex(hit.textureIndex)]
                            .SampleLevel(gSampler, hit.uv, hitLod).rgb;
    // 発光は発光テクスチャが決める。無い物は色のテクスチャ
    float3 hitEmissive = hitTexture;
    if (hit.emissiveTextureIndex != hit.textureIndex && any(hit.emissive > 0.0f))
    {
        hitEmissive = gTextures[NonUniformResourceIndex(hit.emissiveTextureIndex)]
                          .SampleLevel(gSampler, hit.uv, hitLod).rgb;
    }
    float3 sunDirection = float3(0.0f, 1.0f, 0.0f);
    float3 sunRadiance = float3(0.0f, 0.0f, 0.0f);
    if (gLights.numDirectionalLights > 0)
    {
        sunDirection = normalize(-gLights.directionalLights[0].direction);
        sunRadiance = gLights.directionalLights[0].color.rgb * gLights.directionalLights[0].intensity;
    }
    float3 hitSkyColor = gLights.ambientSkyColor;
    float3 hitGroundColor = gLights.ambientGroundColor;
    float hitAmbientIntensity = gLights.ambientIntensity;
    if ((gLights.renderFlags & kRenderFlagGI) != 0 && hit.hasNormal)
    {
        // 映った先もプローブの間接光で照らす
        float hitWeight;
        float3 hitIndirect = DDGIGetIrradiance(hit.position, hit.normal,
                                               hit.toOrigin, gSampler, hitWeight);
        float3 hitAmbient = HemisphereAmbient(hit.normal, gLights.ambientSkyColor,
                                              gLights.ambientGroundColor, gLights.ambientIntensity);
        hitSkyColor = lerp(hitAmbient, hitIndirect, hitWeight);
        hitGroundColor = hitSkyColor;
        hitAmbientIntensity = 1.0f;
    }
    return ShadeReflectionHit(hit, hitTexture, hitEmissive, hitSkyColor, hitGroundColor,
                              hitAmbientIntensity, sunDirection, sunRadiance, gLights.rayTracedShadowMask);
}

PixelShaderOutput main(VertxShaderOutput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTextures[gMaterial.textureIndex].Sample(gSampler, transformedUV.xy);

    if (textureColor.a <= gMaterial.alphaRef)
    {
        discard;
    }

    // レイの原点を浮かせる向きは形状そのものの法線を使う。法線マップ後の向きで浮かせると、
    // 傾きの強い所で原点が面の下へ潜り、自分の面に当たる
    float3 geometricNormal = normalize(input.normal);
    // 両面の素材を裏から見ているときは法線を見ている側へ向ける
    if (gMaterial.isTwoSided != 0 && !isFrontFace)
    {
        geometricNormal = -geometricNormal;
    }
    float3 normal = geometricNormal;

    if (gMaterial.useNormalMap != 0)
    {
        float3 tangent = normalize(input.tangent);
        // Gram-Schmidtで直交化
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        // Bitangentの計算順序を入れ替えてUVのV方向(下向き)に合わせる
        float3 bitangent = normalize(cross(tangent, normal));
        // UV を反転して貼った面でも、従法線がテクスチャの上を向くようにそろえる
        float2 duvdx = ddx(transformedUV.xy);
        float2 duvdy = ddy(transformedUV.xy);
        float uvDeterminant = duvdx.x * duvdy.y - duvdx.y * duvdy.x;
        float3 vDirection = (duvdx.x * ddy(input.WorldPosition) - duvdy.x * ddx(input.WorldPosition)) * sign(uvDeterminant);
        if (abs(uvDeterminant) > 1e-12f && dot(bitangent, vDirection) > 0.0f)
        {
            bitangent = -bitangent;
        }
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        float3 sampledNormal = gTextures[gMaterial.normalMapIndex].Sample(gSampler, transformedUV.xy).rgb;
        sampledNormal = sampledNormal * 2.0f - 1.0f;
        // 接空間からワールド空間へ変換
        normal = normalize(mul(sampledNormal, TBN));
    }

    float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);

    if (gMaterial.enableLighting != 0)
    {
        // テクスチャは sRGB として読み込まれているので、サンプルした時点でリニアになっている
        float3 albedo = gMaterial.color.rgb * textureColor.rgb;
        float metallic = gMaterial.metallic;
        float roughness = gMaterial.roughness;
        if (gMaterial.useMetallicRoughnessMap != 0)
        {
            // glTF の並び。G=粗さ B=金属。マテリアルの値は倍率として掛ける
            float2 roughnessMetallic = gTextures[gMaterial.metallicRoughnessIndex].Sample(gSampler, transformedUV.xy).gb;
            roughness *= roughnessMetallic.x;
            metallic *= roughnessMetallic.y;
        }
        metallic = saturate(metallic);
        // 濡れた路面。屋根の下は濡らさない。動く物 (走る車など) は屋根とみなさない
        WetSurface wetSurface = (WetSurface)0;
        bool isWetCovered = false;
        if (gLights.wetness > 0.0f)
        {
            wetSurface = GetWetSurface(input.WorldPosition, geometricNormal, gLights.wetness, gLights.puddleAmount, gLights.puddleScale);
            if (wetSurface.wet > 0.0f &&
                TraceShadowRayMasked(input.WorldPosition, float3(0.0f, 1.0f, 0.0f), geometricNormal, kWetSkyCheckDistance,
                                     kShadowRayBias, kRayMaskStaticOpaque) <= 0.0f)
            {
                wetSurface = (WetSurface)0;
                isWetCovered = true;
            }
            ApplyWetSurface(wetSurface, geometricNormal, albedo, roughness, metallic, normal);
            ApplyRainRipples(wetSurface, input.WorldPosition, geometricNormal, gLights.rainAmount, gLights.fog.cloudTime, normal);
        }
        // 0 にすると GGX のハイライトが1点に潰れてちらつくので下限を置く
        roughness = clamp(roughness, 0.04f, 1.0f);
        if ((gLights.renderFlags & kRenderFlagSpecularAA) != 0)
        {
            roughness = SpecularAntiAliasing(normal, roughness);
        }

        // 金属はアルベドをそのまま鏡面反射率にし、拡散反射を持たない
        float3 f0 = lerp(kDielectricF0, albedo, metallic);
        float3 diffuseColor = albedo * (1.0f - metallic);

        // 品質の設定で足す鏡面の補正
        bool isEnergyCompensated = (gLights.renderFlags & kRenderFlagEnergyCompensation) != 0;
        float3 energyCompensation = float3(1.0f, 1.0f, 1.0f);
        if (isEnergyCompensated && gLights.lightingMode == kLightingModePBR)
        {
            energyCompensation = EnergyCompensation(f0, saturate(dot(normal, toEye)), roughness, gSampler);
        }

        // 光の当たらない面の明るさはこれが全て(PBRには Half-Lambert のような回り込みが無い)。
        // アンビエントは向きを持たないので、遮蔽はAOで別に掛ける
        float ao = GetAmbientOcclusion(input.position, input.WorldPosition, normal,
                                       gLights.aoMode, gLights.aoSampleCount, gLights.aoRadius, gLights.aoIntensity);
        if (gMaterial.useOcclusionMap != 0)
        {
            // モデルに描き込まれた細かい物陰
            ao *= gTextures[gMaterial.occlusionMapIndex].Sample(gSampler, transformedUV.xy).r;
        }
        float3 ambientColor = GetAmbientDiffuse(normal, gLights.ambientMode, gLights.ambientSkyColor,
                                                gLights.ambientGroundColor, gLights.ambientIntensity,
                                                gLights.iblAmbientScale, gLights.environmentTint, gSampler);
        // アンビエントのうちプローブの光が占める度合い
        float probeWeight = 0.0f;
        if (gBakedGI.volumeCount > 0)
        {
            // 焼いた間接光。置いた箱の中だけ
            float3 skyUpColor = GetAmbientDiffuse(float3(0.0f, 1.0f, 0.0f), gLights.ambientMode, gLights.ambientSkyColor,
                                                  gLights.ambientGroundColor, gLights.ambientIntensity,
                                                  gLights.iblAmbientScale, gLights.environmentTint, gSampler);
            float3 skyDownColor = GetAmbientDiffuse(float3(0.0f, -1.0f, 0.0f), gLights.ambientMode, gLights.ambientSkyColor,
                                                    gLights.ambientGroundColor, gLights.ambientIntensity,
                                                    gLights.iblAmbientScale, gLights.environmentTint, gSampler);
            float bakedWeight;
            float3 baked = BakedGIGetIrradiance(input.WorldPosition, normal, toEye, skyUpColor, skyDownColor, gSampler, bakedWeight);
            ambientColor = lerp(ambientColor, baked, bakedWeight);
            probeWeight = bakedWeight;
        }
        if ((gLights.renderFlags & kRenderFlagGI) != 0)
        {
            // プローブの間接光。格子の外は元のアンビエント
            float giWeight;
            float3 indirect = DDGIGetIrradiance(input.WorldPosition, normal, toEye, gSampler, giWeight);
            ambientColor = lerp(ambientColor, indirect, giWeight);
            probeWeight = giWeight;
        }
        if ((gLights.renderFlags & kRenderFlagScreenGI) != 0 && gMaterial.color.a >= 1.0f)
        {
            // 画面のプローブの間接光。半透明はプリパスに居ないので使わない
            float4 screenGI = SampleScreenGI(input.position);
            ambientColor = lerp(ambientColor, screenGI.rgb, screenGI.a);
            probeWeight = lerp(probeWeight, 1.0f, screenGI.a);
        }
        float3 totalLight = ambientColor * diffuseColor * ao;

        // どのループも寄与が0の光源はレイを飛ばす前に落とす。
        // BRDF は NdotL<=0 で0を返すので、間引いても絵は変わらない

        // Directional Lights
        // 0番の平行光源の影。映り込みを暗くするのに使い回す
        float sunShadow = 1.0f;
        for (int i = 0; i < gLights.numDirectionalLights; i++)
        {
            DirectionalLight light = gLights.directionalLights[i];
            float3 L = normalize(-light.direction);
            float3 radiance = light.color.rgb * light.intensity;
            if (!any(radiance > 0.0f) || dot(normal, L) <= 0.0f)
            {
                continue;
            }

            float shadow = GetDirectionalShadow(input.position, input.WorldPosition, L, geometricNormal,
                                                uint(i), gLights.rayTracedShadowMask, gLights.shadowMode);
            if (i == 0 && gLights.fog.skyMode == kSkyModeAtmosphere)
            {
                // 雲が太陽を遮る
                shadow *= CloudShadow(input.WorldPosition, L, gLights.fog.cloudCoverage, gLights.fog.cloudTime, gLights.fog.cloudShadow);
            }
            radiance *= shadow;
            if (shadow > 0.0f && (gLights.rayTracedShadowMask & kShadowMaskDirectional) != 0)
            {
                radiance *= TranslucentShadow(input.WorldPosition, L, geometricNormal, kShadowRayMaxDistance);
            }
            if (i == 0)
            {
                sunShadow = shadow;
            }

            totalLight += DirectBRDF(normal, toEye, L, diffuseColor, f0, roughness, energyCompensation) * radiance;
        }

        // Point Lights
        for (int j = 0; j < gLights.numPointLights; j++)
        {
            PointLight light = gLights.pointLights[j];
            float3 L = normalize(light.position - input.WorldPosition);
            float3 radiance = light.color.rgb * light.intensity * PointLightAttenuation(light, input.WorldPosition);
            if (!any(radiance > 0.0f) || dot(normal, L) <= 0.0f)
            {
                continue;
            }

            radiance *= TraceShadowPunctual(input.WorldPosition, light.position, geometricNormal,
                                            gLights.rayTracedShadowMask, kShadowMaskPoint, light.sourceRadius);
            if (any(radiance > 0.0f) && (gLights.rayTracedShadowMask & kShadowMaskPoint) != 0)
            {
                float lightDistance = length(light.position - input.WorldPosition);
                radiance *= TranslucentShadow(input.WorldPosition, L, geometricNormal,
                                              max(lightDistance - max(kShadowRayBias, light.sourceRadius), 0.0f));
            }

            totalLight += DirectBRDF(normal, toEye, L, diffuseColor, f0, roughness, energyCompensation) * radiance;
        }

        // Spot Lights
        for (int k = 0; k < gLights.numSpotLights; k++)
        {
            SpotLight light = gLights.spotLights[k];
            float3 lightDirOnSurface = normalize(input.WorldPosition - light.position);
            float3 L = -lightDirOnSurface;
            float3 radiance = light.color.rgb * light.intensity * SpotLightAttenuation(light, input.WorldPosition, lightDirOnSurface);
            if (!any(radiance > 0.0f) || dot(normal, L) <= 0.0f)
            {
                continue;
            }

            radiance *= TraceShadowPunctual(input.WorldPosition, light.position, geometricNormal,
                                            gLights.rayTracedShadowMask, kShadowMaskSpot, 0.0f);
            if (any(radiance > 0.0f) && (gLights.rayTracedShadowMask & kShadowMaskSpot) != 0)
            {
                radiance *= TranslucentShadow(input.WorldPosition, L, geometricNormal,
                                              max(length(light.position - input.WorldPosition) - kShadowRayBias, 0.0f));
            }

            totalLight += DirectBRDF(normal, toEye, L, diffuseColor, f0, roughness, energyCompensation) * radiance;
        }

        // 影なしのライト。自分のクラスタに届くものだけ回す
        if (gClusterParams.lightCount > 0)
        {
            // 設定が On なら AO で物陰を暗くする (接地の影)
            float localLightAO = (gLights.renderFlags & kRenderFlagLocalLightAO) != 0 ? ao : 1.0f;
            float viewZ = mul(float4(input.WorldPosition, 1.0f), gClusterParams.view).z;
            uint cluster = ClusterIndex(input.position.xy, viewZ, gClusterParams);
            uint count = gClusterCounts[cluster];
            for (uint c = 0; c < count; c++)
            {
                ClusteredLight light = gClusteredLights[gClusterIndices[cluster * kMaxLightsPerCluster + c]];
                if (light.type == kClusteredLightTube || light.type == kClusteredLightRect)
                {
                    // 形のある光源。拡散はいちばん近い点、ハイライトは反射の向きにいちばん近い点から
                    float3 Ld;
                    float lightDistance;
                    float shapeAttenuation = ClusteredLightShapeAttenuation(light, input.WorldPosition, Ld, lightDistance);
                    if (shapeAttenuation <= 0.0f)
                    {
                        continue;
                    }
                    float3 shapeRadiance = light.color * shapeAttenuation;
                    // 粗い面はいちばん近い点の光1回で済ませる
                    if (roughness > kShapedLightRoughCutoff)
                    {
                        totalLight += DirectBRDF(normal, toEye, Ld, diffuseColor, f0, roughness, energyCompensation) * shapeRadiance * localLightAO;
                        continue;
                    }
                    float3 specularPoint = ClusteredLightSpecularPoint(light, input.WorldPosition, reflect(-toEye, normal));
                    float specularDistance = length(specularPoint - input.WorldPosition);
                    float3 Ls = specularDistance > 0.0f ? (specularPoint - input.WorldPosition) / specularDistance : Ld;
                    float specularScale = ClusteredLightSpecularNormalization(light, roughness, specularDistance);
                    totalLight += (DirectDiffuse(normal, toEye, Ld, diffuseColor, f0, roughness) * localLightAO +
                                   DirectSpecular(normal, toEye, Ls, f0, roughness, energyCompensation) * specularScale) * shapeRadiance;
                    continue;
                }
                float3 toLight = light.position - input.WorldPosition;
                float distance = length(toLight);
                if (distance >= light.range || distance <= 0.0f)
                {
                    continue;
                }
                float3 L = toLight / distance;
                if (dot(normal, L) <= 0.0f)
                {
                    continue;
                }
                float3 radiance = light.color * ClusteredLightAttenuation(light, distance, L);
                totalLight += DirectBRDF(normal, toEye, L, diffuseColor, f0, roughness, energyCompensation) * radiance * localLightAO;
            }
        }

        // 鏡面の環境光。映り込ませないマテリアルでは計算しない。金属と濡れた面は常に付ける
        float environmentCoefficient = max(max(gMaterial.environmentCoefficient, metallic), wetSurface.wet);
        uint reflectionMode = gLights.reflectionMode;
        if ((gLights.renderFlags & kRenderFlagReflectAll) != 0 && environmentCoefficient < 1.0f)
        {
            // 映り込みを付けていなかったマテリアルにも付ける。レイは映り込みがはっきり見える所だけ
            environmentCoefficient = 1.0f;
            float3 fresnel = F_Schlick(f0, saturate(dot(normal, toEye)));
            float visibleWeight = max(fresnel.r, max(fresnel.g, fresnel.b)) * (1.0f - roughness);
            if (roughness > 0.5f || visibleWeight < 0.1f)
            {
                reflectionMode = kReflectionModeOff;
            }
            else
            {
                reflectionMode = min(reflectionMode, kReflectionModeOcclusion);
            }
        }
        if (environmentCoefficient > 0.0f)
        {
            // 別パスの映り込みを使える画素はレイを飛ばさない
            float4 screenReflection = float4(0.0f, 0.0f, 0.0f, 0.0f);
            if ((gLights.renderFlags & kRenderFlagScreenReflection) != 0 && gMaterial.color.a >= 1.0f)
            {
                screenReflection = SampleScreenReflection(input.position);
            }
            // 屋根の下の画素はプリパスの結果を使わない
            bool useScreenReflection = screenReflection.a > 0.0f && !isWetCovered;

            // 粗い面の映り込みはプローブの光で代える。別パスの映り込みは kMaxRoughness_ まで
            float roughProbeWeight = probeWeight * saturate((roughness - 0.3f) / 0.1f);
            float3 roughProbeColor = ambientColor * ao;

            // 置いた反射プローブ。空の見えるぶんは GetAmbientSpecular が今の空から足す
            float reflectionProbeInfluence = 0.0f;
            float4 reflectionProbe = float4(0.0f, 0.0f, 0.0f, 0.0f);
            if (gReflectionProbeParams.probeCount > 0 && !useScreenReflection)
            {
                reflectionProbe = SampleReflectionProbes(input.WorldPosition, reflect(-toEye, normal), roughness * kPrefilteredMaxMip,
                                                         gSampler, reflectionProbeInfluence);
            }

            // プローブで代えきれる面と、反射プローブの中の遮蔽だけ見るレイは飛ばさない
            uint traceMode = (useScreenReflection || roughProbeWeight >= 1.0f) ? kReflectionModeOff : reflectionMode;
            if (reflectionProbeInfluence >= 1.0f && traceMode == kReflectionModeOcclusion)
            {
                traceMode = kReflectionModeOff;
            }
            ReflectionHit reflectionHit = TraceReflectionHit(input.WorldPosition, reflect(-toEye, normal), normal,
                                                             gLights.reflectionMaxDistance, traceMode);
            float3 occludedColor = ambientColor * ao;
            if (reflectionHit.isHit && reflectionMode == kReflectionModeObjects)
            {
                occludedColor = ShadeForwardReflectionHit(reflectionHit, roughness);
            }
            float3 skyFallback = HemisphereAmbient(reflect(-toEye, normal), gLights.ambientSkyColor,
                                                   gLights.ambientGroundColor, gLights.ambientIntensity);
            float occludedWeight = reflectionHit.isHit ? 1.0f : 0.0f;
            if (reflectionProbeInfluence > 0.0f && !(reflectionHit.isHit && traceMode == kReflectionModeObjects))
            {
                // 物を光らせたレイが無ければ、焼いた景色 (空 × (1 - a) + rgb) へ寄せる
                float probeWeight = lerp(occludedWeight, reflectionProbe.a, reflectionProbeInfluence);
                float3 probeSum = lerp(occludedColor * occludedWeight, reflectionProbe.rgb, reflectionProbeInfluence);
                occludedColor = probeWeight > 1e-4f ? probeSum / probeWeight : float3(0.0f, 0.0f, 0.0f);
                occludedWeight = probeWeight;
            }
            if (traceMode == kReflectionModeObjects)
            {
                // 反射の中のガラス。不透明な物の上に手前のガラスを重ねる
                ReflectionHit glassHit = TraceReflectionGlass(input.WorldPosition, reflect(-toEye, normal), normal,
                                                              reflectionHit.isHit ? reflectionHit.hitDistance : gLights.reflectionMaxDistance);
                if (glassHit.isHit)
                {
                    float3 glassColor = ShadeForwardReflectionHit(glassHit, roughness);
                    float glassWeight = glassHit.opacity;
                    float blendedWeight = occludedWeight + glassWeight - occludedWeight * glassWeight;
                    occludedColor = (occludedColor * occludedWeight * (1.0f - glassWeight) + glassColor * glassWeight) / max(blendedWeight, 1e-4f);
                    occludedWeight = blendedWeight;
                }
            }
            if (useScreenReflection)
            {
                // 空が見えたぶんは GetAmbientSpecular が空から引く
                float skyVisibility = ScreenReflectionSkyVisibility(screenReflection);
                occludedColor = skyVisibility < 0.999f ? screenReflection.rgb / (1.0f - skyVisibility) : float3(0.0f, 0.0f, 0.0f);
                occludedWeight = 1.0f - skyVisibility;
            }
            // 滑らかな面は飛ばしたレイ、粗くなるにつれプローブへ
            occludedColor = lerp(occludedColor, roughProbeColor, roughProbeWeight);
            occludedWeight = lerp(occludedWeight, 1.0f, roughProbeWeight);
            float3 specularEnv = GetAmbientSpecular(normal, toEye, roughness, f0, occludedWeight,
                                                    occludedColor, gLights.ambientMode, gLights.environmentTint,
                                                    gLights.hasEnvironment != 0, skyFallback, isEnergyCompensated, gSampler);

            if ((gLights.renderFlags & kRenderFlagSpecularOcclusion) != 0)
            {
                float3 R = reflect(-toEye, normal);
                specularEnv *= SpecularOcclusion(saturate(dot(normal, toEye)), ao, roughness) * HorizonOcclusion(R, geometricNormal);
            }

            // 金属は拡散反射が無く、映り込みを暗くしないと影がどこにも出ない
            specularEnv *= lerp(1.0f, sunShadow, gLights.reflectionShadow);

            totalLight += specularEnv * environmentCoefficient * gLights.reflectionIntensity;
        }

        output.color.rgb = totalLight;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    // 不透明に描く物は、テクスチャのアルファを抜きの判定にだけ使う
    if (gMaterial.color.a >= 1.0f && gCamera.additive == 0.0f)
    {
        output.color.a = 1.0f;
    }

    // 自分で光るぶん。ライトにも影にも左右されない
    if (gMaterial.useEmissiveMap != 0)
    {
        // 光る所と色はテクスチャが決める
        output.color.rgb += gMaterial.emissive * gTextures[gMaterial.emissiveMapIndex].Sample(gSampler, transformedUV.xy).rgb;
    }
    else
    {
        output.color.rgb += gMaterial.emissive * textureColor.rgb;
    }

    // リムライト。視線に対して寝ている面(輪郭)ほど光る
    if (any(gMaterial.rimColor > 0.0f))
    {
        float rimFacing = saturate(dot(normalize(input.normal), normalize(gCamera.worldPosition - input.WorldPosition)));
        output.color.rgb += gMaterial.rimColor * pow(1.0f - rimFacing, max(gMaterial.rimPower, 0.01f));
    }

    // 空気遠近とフォグ。ライティングしない物にも掛ける
    output.color.rgb = ApplyAerialPerspective(gLights.fog, output.color.rgb, input.WorldPosition - gCamera.worldPosition, gCamera.additive);
    float fogAmount = FogAmount(gLights.fog, gCamera.worldPosition, input.WorldPosition);
    if (fogAmount > 0.0f)
    {
        float3 fogColor = FogColor(gLights.fog, input.WorldPosition - gCamera.worldPosition);
        output.color.rgb = ApplyFog(output.color.rgb, fogColor, fogAmount, gCamera.additive);
    }

    // 屈折。不透明の後に描く物 (a が1未満) だけ、奥の物をずらして自分で混ぜる
    if (refractionEnabled != 0 && gMaterial.color.a < 1.0f && gCamera.additive == 0.0f)
    {
        float3 behind = SampleRefraction(input.WorldPosition, normal, toEye, input.position, gSampler);
        output.color.rgb = lerp(behind, output.color.rgb, saturate(output.color.a));
        output.color.a = 1.0f;
    }

    return output;
}
