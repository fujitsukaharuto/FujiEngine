Texture2D InputTexture : register(t0);
SamplerState gSampler : register(s0);
RWTexture2D<float4> outputTexture : register(u0);

float Luminance(float3 v)
{
    return dot(v, float3(0.2125f, 0.7154f, 0.0721f));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    int width, height;
    InputTexture.GetDimensions(width, height);
    if (coord.x >= width || coord.y >= height)
        return;

    static const float2 kIndex3x3[3][3] =
    {
        { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
        { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
    };

    static const float kPrewittHorizontalKernel[3][3] =
    {
        { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
        { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
        { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    };
    static const float kPrewittVerticalKernel[3][3] =
    {
        { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
    };

    float2 texcoord = (coord + 0.5f) / float2(width, height);
    float2 uvStepSize = float2(rcp(float(width)), rcp(float(height)));

    float2 difference = float2(0.0f, 0.0f);

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 offset = kIndex3x3[x][y] * uvStepSize;
            float2 sampleUV = texcoord + offset;
            float3 fetchColor = InputTexture.SampleLevel(gSampler, sampleUV, 0).rgb;
            float luminance = Luminance(fetchColor);
            
            difference.x += luminance * kPrewittHorizontalKernel[x][y];
            difference.y += luminance * kPrewittVerticalKernel[x][y];
        }
    }

    float weight = saturate(length(difference) * 6.0f);

    float3 baseColor = InputTexture.SampleLevel(gSampler, texcoord, 0).rgb;
    float3 finalColor = (1.0f - weight) * baseColor;
    
    outputTexture[coord] = float4(finalColor, 1.0f);
}