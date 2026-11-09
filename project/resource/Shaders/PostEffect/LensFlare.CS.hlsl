// レンズフレア。太陽から画面の中心を通る線の上に光の玉 (ゴースト) を並べる

Texture2D InputTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

// OffscreenManager.h の LensFlareParams と並びが一致していること
cbuffer LensFlareParams : register(b0)
{
    float2 sunUV; // 太陽の画面上の位置
    float intensity; // 0以下なら何もしない
    float pad;
    float3 color; // 太陽の色
    float pad2;
};

// ゴーストの並び。位置は 1 で画面の中心、2 で反対側
static const int kGhostCount = 6;
static const float kGhostOffset[kGhostCount] = { 0.35f, 0.7f, 1.15f, 1.45f, 1.8f, 2.3f };
static const float kGhostRadius[kGhostCount] = { 0.03f, 0.06f, 0.025f, 0.09f, 0.045f, 0.14f };
static const float3 kGhostTint[kGhostCount] =
{
    float3(1.0f, 0.6f, 0.3f), float3(0.4f, 0.8f, 1.0f), float3(1.0f, 0.9f, 0.5f),
    float3(0.5f, 1.0f, 0.6f), float3(1.0f, 0.5f, 0.8f), float3(0.4f, 0.6f, 1.0f),
};
// 太陽が隠れたかを見る範囲 (画面の高さが1) と、縦横の点の数
static const float kOcclusionRadius = 0.01f;
static const int kOcclusionTaps = 5;

/// <summary>太陽が見えている割合。画面の外の点は数えない</summary>
float SunVisibility(float2 size)
{
    float visible = 0.0f;
    float count = 0.0f;
    float2 step = kOcclusionRadius * float2(size.y / size.x, 1.0f) * (2.0f / (kOcclusionTaps - 1));
    for (int y = 0; y < kOcclusionTaps; ++y)
    {
        for (int x = 0; x < kOcclusionTaps; ++x)
        {
            float2 p = sunUV + (float2(x, y) - (kOcclusionTaps - 1) * 0.5f) * step;
            if (all(p >= 0.0f) && all(p < 1.0f))
            {
                visible += gDepthTexture[uint2(p * size)] >= 1.0f ? 1.0f : 0.0f;
                count += 1.0f;
            }
        }
    }
    return count > 0.0f ? visible / count : 0.0f;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    uint width, height;
    InputTexture.GetDimensions(width, height);
    if (coord.x >= width || coord.y >= height)
    {
        return;
    }

    float3 scene = InputTexture[coord].rgb;
    if (intensity <= 0.0f)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }

    float2 size = float2(width, height);
    float visibility = SunVisibility(size);
    if (visibility <= 0.0f)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }

    float2 uv = (coord + 0.5f) / size;
    float2 aspect = float2(size.x / size.y, 1.0f);
    float2 toCenter = float2(0.5f, 0.5f) - sunUV;

    float3 flare = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < kGhostCount; ++i)
    {
        float2 center = sunUV + toCenter * kGhostOffset[i];
        float distance = length((uv - center) * aspect) / kGhostRadius[i];
        // 縁がわずかに明るいぼやけた円
        float disc = 1.0f - smoothstep(0.7f, 1.0f, distance);
        float rim = exp(-pow((distance - 0.9f) * 8.0f, 2.0f)) * 0.5f;
        // 大きい玉ほど薄い
        flare += kGhostTint[i] * (disc * 0.6f + rim) * (0.03f / kGhostRadius[i]);
    }
    // 画面の端ほど弱める
    float2 edge = abs(sunUV - 0.5f) * 2.0f;
    float fade = saturate(1.5f - max(edge.x, edge.y) * 1.2f);

    outputTexture[coord] = float4(scene + flare * color * (intensity * visibility * fade * 0.1f), 1.0f);
}
