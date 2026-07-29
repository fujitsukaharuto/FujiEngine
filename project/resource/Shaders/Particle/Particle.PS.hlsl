#include "Particle.hlsli"

struct Material
{
    float4 color;
    float4x4 uvTransform;
    int enableLighting;
    float shininess;
    float alphaRef;
    float environmentCoefficient;
};

//struct DirectionalLight
//{
//    float4 color;
//    float3 direction;
//    float intensity;
//};

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);
//ConstantBuffer<DirectionalLight> gDirectionnalLight : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float4x4 CreateUVMatrix(float2 uvScale, float2 uvTrans)
{
    return float4x4(
        uvScale.x, 0.0f, 0.0f, 0.0f,
        0.0f, uvScale.y, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        uvTrans.x, uvTrans.y, 0.0f, 1.0f
    );
}


PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4x4 uvTransform = CreateUVMatrix(input.uvScale, input.uvTrans);
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    output.color = input.color * textureColor;
    if (output.color.a <= 0.15)
    {
        discard;
    }
    return output;
}