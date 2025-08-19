cbuffer VignetteConstantBuffer : register(b0)
{
    float3 vignetteColor; // R, G, B の輝度係数
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
    uint2 pixel = dispatchThreadID.xy;

    // テクスチャ座標 (0~1)
    int width, height;
    outputTexture.GetDimensions(width, height);

    // 入力テクスチャから色を読み込む
    float2 uv = (pixel + 0.5) / float2(width, height);

    // 中心からの距離
    float2 center = float2(0.5, 0.5);
    float dist = distance(uv, center);

    // ビネット係数を計算
    float vignette = 1.0 - smoothstep(0.5, 0.9, dist);
    vignette = lerp(1.0, vignette, 0.8);

    // 元の色取得
    float4 color = InputTexture.SampleLevel(InputSampler, uv, 0);

    // ビネット適用
    color.rgb = lerp(vignetteColor, color.rgb, vignette);
    color.rgb = LinearToSRGB(color.rgb);

    // 書き込み
    outputTexture[pixel] = color;
}
