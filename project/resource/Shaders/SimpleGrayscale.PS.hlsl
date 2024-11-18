Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    //int particleCount = 3;
    //float threshold = 1.0f;
    //float2 particles[3] =
    //{
    //    { 300.0f, 300.0f },
    //    { 800.0f, 300.0f },
    //    { 300.0f, 700.0f }
    //};
    //float radius[3] = { 300.0f, 100.0f, 50.0f };
    //float2 pixelPos = input.position.xy;
    //float4 color = g_InputTexture.Sample(g_Sampler, input.texcoord);
    //float field = 0.0;
    //for (int i = 0; i < particleCount; ++i)
    //{
    //    float2 diff = pixelPos - particles[i];
    //    field += radius[i] * radius[i] / dot(diff, diff);
    //}

    //if (field > threshold)
    //{
    //    return float4(1.0, 1.0, 1.0, 1.0); // メタボールの部分を白で描画
    //}
    //else
    //{
    //    return color; // それ以外の部分
    //}
    
    
    
    float4 color = g_InputTexture.Sample(g_Sampler, input.texcoord);
    float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
    float3 result = lerp(color.rgb, float3(gray, gray, gray), 0.7);
    return float4(result, 1.0);
}