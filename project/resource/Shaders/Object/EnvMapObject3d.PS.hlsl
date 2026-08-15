#include "Object3d.hlsli"
#include "../Common/PBR.hlsli"
#include "../Common/RayTracedShadow.hlsli"
#include "../Common/RayTracedAO.hlsli"
#include "../Common/ScreenSpaceAO.hlsli"
#include "../Common/RayTracedReflection.hlsli"

// Object3d.PS との違いは、スカイボックスのキューブマップを鏡面の環境光として足す点だけ。
// 構造体(Material / 各種ライト / Camera)は Object3d.hlsli にまとめてある

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTextures[] : register(t0, space0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b2);
ConstantBuffer<Camera> gCamera : register(b3);
// gEnvironment の宣言は RayTracedReflection.hlsli に集約してある

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 減衰カーブは旧実装のものをそのまま使う(Object3d.PS と同じ理由)
float PointLightAttenuation(PointLight light, float3 worldPos)
{
    float distance = length(light.position - worldPos);
    return pow(saturate(-distance / light.radius + 1.0f), light.decay);
}

float SpotLightAttenuation(SpotLight light, float3 worldPos, float3 lightDirOnSurface)
{
    float distance = length(light.position - worldPos);
    float attenuation = pow(saturate(-distance / light.distance + 1.0f), light.decay);

    float cosAngle = dot(lightDirOnSurface, normalize(light.direction));
    float falloff = saturate((cosAngle - light.cosAngle) / (light.cosStart - light.cosAngle));

    return attenuation * falloff;
}

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTextures[gMaterial.textureIndex].Sample(gSampler, transformedUV.xy);

    if (textureColor.a <= gMaterial.alphaRef)
    {
        discard;
    }

    float3 normal = normalize(input.normal);

    if (gMaterial.useNormalMap != 0)
    {
        float3 tangent = normalize(input.tangent);
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        float3 bitangent = normalize(cross(tangent, normal));
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        float3 sampledNormal = gTextures[gMaterial.normalMapIndex].Sample(gSampler, transformedUV.xy).rgb;
        sampledNormal = sampledNormal * 2.0f - 1.0f;
        normal = normalize(mul(sampledNormal, TBN));
    }

    float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);

    if (gMaterial.enableLighting != 0)
    {
        float3 albedo = gMaterial.color.rgb * textureColor.rgb;
        float metallic = saturate(gMaterial.metallic);
        float roughness = clamp(gMaterial.roughness, 0.04f, 1.0f);

        float3 f0 = lerp(kDielectricF0, albedo, metallic);
        float3 diffuseColor = albedo * (1.0f - metallic);

        float ao = GetAmbientOcclusion(input.position, input.WorldPosition, normal,
                                       gLights.aoMode, gLights.aoSampleCount, gLights.aoRadius, gLights.aoIntensity);
        float3 totalLight = HemisphereAmbient(normal, gLights.ambientSkyColor,
                                              gLights.ambientGroundColor, gLights.ambientIntensity) * diffuseColor * ao;

        // ライトのループは Object3d.PS と同じ形(寄与が0の光源はレイを飛ばす前に落とす)
        for (int i = 0; i < gLights.numDirectionalLights; i++)
        {
            DirectionalLight light = gLights.directionalLights[i];
            float3 L = normalize(-light.direction);
            float3 radiance = light.color.rgb * light.intensity;
            if (!any(radiance > 0.0f) || dot(normal, L) <= 0.0f)
            {
                continue;
            }

            radiance *= TraceShadowDirectional(input.WorldPosition, L, normal, gLights.rayTracedShadowMask);

            totalLight += BRDF(normal, toEye, L, diffuseColor, f0, roughness) * radiance;
        }

        for (int j = 0; j < gLights.numPointLights; j++)
        {
            PointLight light = gLights.pointLights[j];
            float3 L = normalize(light.position - input.WorldPosition);
            float3 radiance = light.color.rgb * light.intensity * PointLightAttenuation(light, input.WorldPosition);
            if (!any(radiance > 0.0f) || dot(normal, L) <= 0.0f)
            {
                continue;
            }

            radiance *= TraceShadowPunctual(input.WorldPosition, light.position, normal,
                                            gLights.rayTracedShadowMask, kShadowMaskPoint);

            totalLight += BRDF(normal, toEye, L, diffuseColor, f0, roughness) * radiance;
        }

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

            radiance *= TraceShadowPunctual(input.WorldPosition, light.position, normal,
                                            gLights.rayTracedShadowMask, kShadowMaskSpot);

            totalLight += BRDF(normal, toEye, L, diffuseColor, f0, roughness) * radiance;
        }

        // 鏡面の環境光。本来は粗さでミップを選ぶ prefiltered map を引く所の暫定版
        float3 ambientColor = HemisphereAmbient(normal, gLights.ambientSkyColor,
                                                gLights.ambientGroundColor, gLights.ambientIntensity);
        float3 environmentColor = TraceReflection(input.WorldPosition, normal, toEye, ambientColor * ao,
                                                  gLights.reflectionMaxDistance, gLights.enableReflection, gSampler);

        float NdotV = saturate(dot(normal, toEye));
        float3 fresnel = F_Schlick(f0, NdotV);
        // 粗い面ほど映り込みがぼやけて弱く見えるので、その代用として単純に落とす
        float3 specularEnv = environmentColor * fresnel * (1.0f - roughness);

        totalLight += specularEnv * gMaterial.environmentCoefficient * gLights.reflectionIntensity;

        output.color.rgb = totalLight;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }

    return output;
}
