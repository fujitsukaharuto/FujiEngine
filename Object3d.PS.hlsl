#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionnalLight : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionnalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        //float cos = saturate(dot(normalize(input.normal), -gDirectionnalLight.direction));
        output.color = gMaterial.color * textureColor * gDirectionnalLight.color * cos * gDirectionnalLight.intensity;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    return output;
}