Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    // 1/4解像度から拡大するので、サンプラのWRAPで端が反対側を拾わないよう内側へ寄せる
    float2 size;
    g_InputTexture.GetDimensions(size.x, size.y);
    float2 uv = clamp(input.texcoord, 0.5f / size, 1.0f - 0.5f / size);

    float4 color = g_InputTexture.Sample(g_Sampler, uv);
    return color;
}