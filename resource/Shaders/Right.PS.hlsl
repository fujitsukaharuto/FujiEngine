
struct ParticleDateBuffer
{
    float4 pos; // 最大100個の粒子
    float4 radius; // 粒子ごとの影響範囲
};

Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);
ConstantBuffer<ParticleDateBuffer> g_Particle : register(b0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 pixelPos = input.position.xy;
    float4 color = g_InputTexture.Sample(g_Sampler, input.texcoord);
    float field = 0.0;

    float2 diff = pixelPos - g_Particle.pos.xy;
    field = length(diff);


    if (field <= g_Particle.radius.x)
    {
        return color;
    }
    else
    {
        return float4(0.04, 0.04, 0.04, 1.0); // それ以外の部分
    }

}