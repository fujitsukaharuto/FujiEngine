// 入力テクスチャ (SRV)
Texture2D InputTexture : register(t0);
// サンプラー
SamplerState gSampler : register(s0);
// 出力テクスチャ (UAV)
RWTexture2D<float4> outputTexture : register(u0);

cbuffer BlerConstantBuffer : register(b0)
{
    float2 center;
    float blurWidth;
};

float3 LinearToSRGB(float3 linearColor)
{
    float3 srgbColor = linearColor;
    // R成分
    if (linearColor.x <= 0.0031308f)
        srgbColor.x = 12.92f * linearColor.x;
    else
        srgbColor.x = 1.055f * pow(linearColor.x, 1.0f / 2.4f) - 0.055f;

    // G成分
    if (linearColor.y <= 0.0031308f)
        srgbColor.y = 12.92f * linearColor.y;
    else
        srgbColor.y = 1.055f * pow(linearColor.y, 1.0f / 2.4f) - 0.055f;

    // B成分
    if (linearColor.z <= 0.0031308f)
        srgbColor.z = 12.92f * linearColor.z;
    else
        srgbColor.z = 1.055f * pow(linearColor.z, 1.0f / 2.4f) - 0.055f;
    return saturate(srgbColor);
}

[numthreads(8, 8, 1)] // スレッドグループのサイズ (X, Y, Z)
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // 現在のピクセルの座標
    int2 coord = dispatchThreadID.xy;
    int width, height;
    InputTexture.GetDimensions(width, height);
    float2 texcoord = (coord + 0.5f) / float2(width, height);
    const int kNumSamples = 10;
    
    float2 direction = texcoord - center;
    float3 outputColor = float3(0.0f, 0.0f, 0.0f);
    
    for (int sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        float2 sampleUV = texcoord + direction * blurWidth * float(sampleIndex);
        outputColor.rgb += InputTexture.SampleLevel(gSampler, sampleUV, 0).rgb;
    }
    outputColor.rgb *= rcp(float(kNumSamples));
    outputColor.rgb = LinearToSRGB(outputColor.rgb);
    // 結果を出力テクスチャに書き込む
    outputTexture[coord] = float4(outputColor, 1.0);
}
