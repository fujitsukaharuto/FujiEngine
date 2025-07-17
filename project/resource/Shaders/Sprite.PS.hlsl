#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
    float alphaRef;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    if (textureColor.a == 0.0)
    {
        discard;
    }
    if (textureColor.a <= gMaterial.alphaRef)
    {
        discard;
    }
    
    textureColor.rgb = pow(textureColor.rgb, 2.2);
    output.color = gMaterial.color * textureColor;
    if (output.color.a == 0.0)
    {
        discard;
    }
    
	return output;
}