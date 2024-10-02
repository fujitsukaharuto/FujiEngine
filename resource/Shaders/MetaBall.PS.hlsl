
struct ParticleDateBuffer
{
    float4 particles[400]; // 最大100個の粒子
    float4 radius[400]; // 粒子ごとの影響範囲
    int particleCount; // 粒子の数
    float threshold; // しきい値
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
    //if (pixelPos.x > 600.0 || pixelPos.y > 600.0)
    //{
    //    return color;
    //}
    float field = 0.0;
    for (int i = 0; i <= g_Particle.particleCount; ++i)
    {
        float2 diff = pixelPos - g_Particle.particles[i].xy;
        field += g_Particle.radius[i].x * g_Particle.radius[i].x / dot(diff, diff);
    }

    if (field > g_Particle.threshold)
    {
        return float4(0.0, 0.0, 1.0, 1.0); // メタボールの部分を白で描画
    }
    else
    {
        return color; // それ以外の部分
    }
    
    //if (field > g_Particle.threshold)
    //{
    //    return color; // メタボールの部分を白で描画
    //}
    //else
    //{
    //    return float4(0.0, 0.0, 0.0, 1.0);; // それ以外の部分
    //}
}