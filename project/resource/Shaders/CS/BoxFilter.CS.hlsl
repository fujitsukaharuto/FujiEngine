Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);
RWTexture2D<float4> outputTexture : register(u0);

float3 LinearToSRGB(float3 linearColor)
{
    float3 srgbColor = linearColor;
    srgbColor = (linearColor <= 0.0031308f) ? (12.92f * linearColor) : (1.055f * pow(linearColor, 1.0f / 2.4f) - 0.055f);
    return saturate(srgbColor);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    int width, height;
    gTexture.GetDimensions(width, height);
    if (coord.x >= width || coord.y >= height)
        return;

    static const float2 kIndex3x3[3][3] =
    {
        { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
        { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
    };

    static const float kKernel3x3[3][3] =
    {
        { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
        { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
        { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    };

    float2 texcoord = (coord + 0.5f) / float2(width, height);
    float2 uvStepSize = float2(rcp(float(width)), rcp(float(height)));
    float3 baseColor = float3(0.0f, 0.0f, 0.0f);

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 offset = kIndex3x3[x][y] * uvStepSize;
            float2 sampleUV = texcoord + offset;
            float3 fetchColor = gTexture.SampleLevel(gSampler, sampleUV, 0).rgb;

            baseColor.rgb += fetchColor * kKernel3x3[x][y];
        }
    }

    float3 finalColor = baseColor;
    finalColor.rgb = LinearToSRGB(finalColor.rgb);
    
    outputTexture[coord] = float4(finalColor, 1.0f);
}