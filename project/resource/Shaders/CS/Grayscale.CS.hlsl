cbuffer GrayscaleConstantBuffer : register(b0)
{
    float3 luminanceFactors; // R, G, B の輝度係数
};

// 入力テクスチャ (SRV)
Texture2D InputTexture : register(t0);

// サンプラー
SamplerState InputSampler : register(s0);

// 出力テクスチャ (UAV)
RWTexture2D<float4> outputTexture : register(u0);


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
    int2 coords = dispatchThreadID.xy;

    // 出力テクスチャのサイズを取得
    int width, height;
    outputTexture.GetDimensions(width, height);

    // 入力テクスチャから色を読み込む
    float4 color = InputTexture.SampleLevel(InputSampler, (float2) coords / float2(width, height), 0.0f);

    // 輝度を計算 (Rec.709)
    float luminance = dot(color.rgb, luminanceFactors);

    // グレースケール値をRGBAに設定
    float3 grayscaleColor = lerp(color.rgb, float3(luminance, luminance, luminance), 0.7);
    grayscaleColor.rgb = LinearToSRGB(grayscaleColor.rgb);
    
    // 結果を出力テクスチャに書き込む
    outputTexture[coords] = float4(grayscaleColor, 1.0);
}
