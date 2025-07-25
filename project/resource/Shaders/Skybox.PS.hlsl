#include "Skybox.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
};

ConstantBuffer<Material> gMaterial : register(b0);
TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);


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

    float exposure = 0.25;
    litColor *= exposure;
    litColor = ToneMapACES(litColor);

    output.color = float4(litColor, textureColor.a * gMaterial.color.a);
    //output.color = textureColor * gMaterial.color;
    return output;
}