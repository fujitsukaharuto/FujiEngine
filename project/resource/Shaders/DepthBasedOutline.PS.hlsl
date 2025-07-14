Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSamplerPoint : register(s1);
cbuffer gMaterial : register(b0)
{
    float4x4 projectionInverse;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


static const float2 kIndex3x3[3][3] =
{
    {{ -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f }},
    {{ -1.0f,  0.0f }, { 0.0f,  0.0f }, { 1.0f,  0.0f }},
    {{ -1.0f,  1.0f }, { 0.0f,  1.0f }, { 1.0f,  1.0f } },
};
static const float kKernel3x3[3][3] =
{
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
};
static const float kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f},
};
static const float kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f},
    { 0.0f, 0.0f, 0.0f},
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f},
};

PixelShaderOutput main(PSInput input)
{
    int width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(rcp(float(width)), rcp(float(height)));
    float2 difference = float2(0.0f, 0.0f);
    
    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
            float viewZ = viewSpace.z * rcp(viewSpace.w);
            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }
    
    float weight = length(difference);
    weight = saturate(weight * 6.0f);
    
    PixelShaderOutput output;
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
	return output;
}