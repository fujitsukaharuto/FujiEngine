#include "CSParticle.hlsli"

Texture2D<float4> gTexture : register(t4);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = input.color * textureColor;
    if (output.color.a <= 0.15)
    {
        discard;
    }
    return output;
}