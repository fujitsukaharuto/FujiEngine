Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float Gaussian(float x, float sigma)
{
    return exp(-0.5 * (x * x) / (sigma * sigma)) / (sqrt(2.0 * 3.14159265) * sigma);
}

float4 main(PSInput input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 0);
    float totalWeight = 0.0;
    int radius = 5; // Blur radius
    float sigma = 4.0f;
    float2 texSize = { 0.00078125f, 0.00138888f };
    
    // Horizontal blur
    for (int i = -radius; i <= radius; ++i)
    {
        float weight = Gaussian(float(i), sigma);
        color += g_InputTexture.Sample(g_Sampler, input.texcoord + float2(i * texSize.x, i*texSize.y)) * weight;
        totalWeight += weight;
    }

    // Normalize the color
    color /= totalWeight;

    return color;
}