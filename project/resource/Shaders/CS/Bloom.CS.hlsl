cbuffer BloomParams : register(b0)
{
    float bloomThreshold; // しきい値（例：1.0）
    float bloomIntensity; // ブルーム強度（例：1.2）
};

Texture2D<float4> gTexture : register(t0); // HDR シーン
SamplerState gSampler : register(s0);
RWTexture2D<float4> outputTexture : register(u0); // 合成結果（Scene + Bloom）

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

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= 1280 || dispatchThreadID.y >= 720)
        return;

    float2 uv = (dispatchThreadID.xy + 0.5f) / float2(1280, 720);

    // 元のシーンカラーを取得（合成のベース）
    float3 sceneColor = gTexture.SampleLevel(gSampler, uv, 0).rgb;

    // =========================
    // Step 1: Bright Pass 抽出
    // =========================
    float luminance = dot(sceneColor, float3(0.2126, 0.7152, 0.0722));
    float3 brightColor = (luminance > bloomThreshold) ? sceneColor : float3(0, 0, 0);

    // =========================
    // Step 2: ガウスブラー（5x5）
    // =========================
    float kernel[5] = { 0.0625, 0.25, 0.375, 0.25, 0.0625 };

    float3 bloom = float3(0, 0, 0);
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            float2 offset = float2(x / (float) 1280, y / (float) 720);
            float3 sampleColor = gTexture.SampleLevel(gSampler, uv + offset, 0).rgb;

            // BrightPass抽出を各サンプルにも適用
            float lum = dot(sampleColor, float3(0.2126, 0.7152, 0.0722));
            float3 bright = (lum > bloomThreshold) ? sampleColor : float3(0, 0, 0);

            bloom += bright * kernel[x + 2] * kernel[y + 2];
        }
    }

    // =========================
    // Step 3: 合成
    // =========================
    float3 finalColor = sceneColor + bloom * bloomIntensity;
    finalColor = LinearToSRGB(finalColor);
    outputTexture[dispatchThreadID.xy] = float4(finalColor, 1.0f);
}