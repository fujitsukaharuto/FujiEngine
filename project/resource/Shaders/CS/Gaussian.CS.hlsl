Texture2D<float4> g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);
RWTexture2D<float4> g_OutputTexture : register(u0);

float3 LinearToSRGB(float3 linearColor)
{
    float3 srgbColor = linearColor;
    srgbColor = (linearColor <= 0.0031308f) ? (12.92f * linearColor) : (1.055f * pow(linearColor, 1.0f / 2.4f) - 0.055f);
    return saturate(srgbColor);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 coord = int2(dispatchThreadID.xy);

    float2 texelSize = float2(0.00078125f, 0.00138888f); // 1.0 / texture size（1280x720の場合）
    int radius = 5;
    float sigma = 4.0f;
    bool horizontal = true; // 横方向ブラー：true、縦方向ブラー：false

    float4 color = float4(0, 0, 0, 0);
    float totalWeight = 0.0;

    for (int i = -radius; i <= radius; ++i)
    {
        float weight = exp(-0.5 * (i * i) / (sigma * sigma)) / (sqrt(6.2831853) * sigma);
        
        float2 offset = horizontal
            ? float2(i * texelSize.x, 0)
            : float2(0, i * texelSize.y);

        float2 uv = (coord + offset / texelSize) * texelSize;

        color += g_InputTexture.SampleLevel(g_Sampler, uv, 0.0f) * weight;
        totalWeight += weight;
    }

    float4 finalColor = color / totalWeight;
    finalColor.rgb = LinearToSRGB(finalColor.rgb);
    
    g_OutputTexture[coord] = finalColor;
}