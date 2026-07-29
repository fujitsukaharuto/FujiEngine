Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = g_InputTexture.Sample(g_Sampler, input.texcoord);
    return color;
}