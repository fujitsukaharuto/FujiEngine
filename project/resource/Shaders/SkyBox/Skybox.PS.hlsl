#include "Skybox.hlsli"
#include "../Common/Fog.hlsli"
#include "../Common/Atmosphere.hlsli"

struct Material
{
    float4 color;
    float4x4 uvTransform;
    int enableLighting;
    float roughness;
    float alphaRef;
    float environmentCoefficient;
};

ConstantBuffer<Material> gMaterial : register(b1);
TextureCube<float4> gTexture : register(t0);
// 計算する空のときに引く、カメラから見た空の数表
Texture2D<float4> gSkyViewLut : register(t1);
SamplerState gSampler : register(s0);
ConstantBuffer<FogParams> gFog : register(b2);


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float3 ToneMapACES(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    float3 litColor = textureColor.rgb * gMaterial.color.rgb;

    if (gFog.skyMode == kSkyModeAtmosphere)
    {
        float3 direction = normalize(input.texcoord);
        // 数表は太陽の明るさ込み。円盤は入っていない
        float3 sky = gSkyViewLut.SampleLevel(gSampler, SkyViewDirectionToUv(direction), 0.0f).rgb;
        litColor = sky + AtmosphereSunDisk(direction, gFog.sunDirection, gFog.sunDiskColor);
        // 星は太陽が約6度沈んでから出す
        if (gFog.starIntensity > 0.0f)
        {
            float night = smoothstep(0.0f, -0.1f, gFog.sunDirection.y);
            litColor += StarField(direction, gFog.cloudTime) * (gFog.starIntensity * night);
        }
        if (gFog.moonIntensity > 0.0f)
        {
            float3 moonDirection = -gFog.sunDirection;
            litColor += MoonDisk(direction, moonDirection) * (gFog.moonIntensity * smoothstep(-0.05f, 0.05f, moonDirection.y));
        }
        // 雲は円盤ごと覆う
        float3 zenith = gSkyViewLut.SampleLevel(gSampler, SkyViewDirectionToUv(float3(0.0f, 1.0f, 0.0f)), 0.0f).rgb;
        float4 cloud = CloudLayer(direction, gFog.sunDirection, gFog.sunDiskColor, zenith, sky,
                                  gFog.cloudCoverage, gFog.cloudTime, gFog.cameraPosition.xz);
        // 雲の底は街の光で照らされる
        cloud.rgb += gFog.skyGlowColor * 0.5f;
        litColor = lerp(litColor, cloud.rgb, cloud.a);
        litColor *= gMaterial.color.rgb;
    }
    else
    {
        // この露光とトーンマップは SkyBox::GetHorizonColor() が同じ計算をしている。変えたら向こうも合わせること
        float exposure = 0.25;
        litColor *= exposure;
        litColor = ToneMapACES(litColor);
    }

    // 空気(フォグ)。遠くの地面が霞んで溶け込む先の色と、地平線の色を揃える
    litColor = lerp(litColor, FogColor(gFog, input.texcoord), FogAmountSky(gFog, normalize(input.texcoord)));

    // 光害。地平線に近いほど街の光で明るい
    float glowHeight = max(normalize(input.texcoord).y, 0.0f) / gFog.skyGlowHeight;
    litColor += gFog.skyGlowColor * exp(-glowHeight * glowHeight);

    output.color = float4(litColor, textureColor.a * gMaterial.color.a);
    //output.color = textureColor * gMaterial.color;
    return output;
}