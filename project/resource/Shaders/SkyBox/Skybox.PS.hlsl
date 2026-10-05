#include "Skybox.hlsli"
#include "../Common/Fog.hlsli"

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

    // この露光とトーンマップは SkyBox::GetHorizonColor() が同じ計算をしている。変えたら向こうも合わせること
    float exposure = 0.25;
    litColor *= exposure;
    litColor = ToneMapACES(litColor);

    // 空気(フォグ)。遠くの地面が霞んで溶け込む先の色と、地平線の色を揃える
    litColor = lerp(litColor, FogColor(gFog, input.texcoord), FogAmountSky(gFog, normalize(input.texcoord)));

    output.color = float4(litColor, textureColor.a * gMaterial.color.a);
    //output.color = textureColor * gMaterial.color;
    return output;
}