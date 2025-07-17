Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSamplerPoint : register(s1);

cbuffer gMaterial : register(b0)
{
    float4x4 projectionInverse;
};

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

    float2 texcoord = (coord + 0.5f) / float2(width, height);
    float2 uvStepSize = float2(1.0f / width, 1.0f / height);

    float2 difference = float2(0.0f, 0.0f);

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

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 offset = kIndex3x3[x][y] * uvStepSize;
            float2 sampleUV = texcoord + offset;

            // サンプリング範囲外のチェック（必要なら）
            sampleUV = clamp(sampleUV, float2(0.0f, 0.0f), float2(1.0f, 1.0f));

            float ndcDepth = gDepthTexture.SampleLevel(gSamplerPoint, sampleUV, 0);
            float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
            float viewZ = viewSpace.z / viewSpace.w;

            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }

    float weight = saturate(length(difference) * 6.0f);

    float3 baseColor = gTexture.SampleLevel(gSampler, texcoord, 0).rgb;
    float3 finalColor = (1.0f - weight) * baseColor;
    finalColor.rgb = LinearToSRGB(finalColor.rgb);
    
    outputTexture[coord] = float4(finalColor, 1.0f);
}