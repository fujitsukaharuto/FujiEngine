#include "BloomPass.hlsli"

// ブルームの縮小。入力を半分の大きさへぼかしながら縮める。
// 最初の1回だけは、明るい所の抽出も兼ねる

Texture2D<float4> gBloomSource : register(t0);
RWTexture2D<float4> gBloomOutput : register(u0);

/// <summary>1点引く。最初の縮小ではここで明るい所だけを残す</summary>
float3 Fetch(float2 uv)
{
    // 異常な値(負や極端に大きい値)が1画素でもあると、ぼかした先の全体へ広がる
    float3 color = clamp(SampleClamped(gBloomSource, uv), 0.0f, 64.0f);

    if (prefilter != 0)
    {
        // しきい値でいきなり切ると、境目の画素が点いたり消えたりしてちらつく
        float luminance = dot(color, kLumaWeight);
        color *= smoothstep(threshold - knee, threshold + knee, luminance);
    }
    return color;
}

/// <summary>4点の平均。最初の縮小では、極端に明るい点が1つあっても引っ張られにくい重みを付ける</summary>
/// <remarks>小さくて強い光(粒子1つなど)が、動くたびに大きな光の塊として瞬くのを防ぐ</remarks>
float4 WeightedBox(float3 a, float3 b, float3 c, float3 d, float coefficient)
{
    float3 average = (a + b + c + d) * 0.25f;
    float weight = coefficient;
    if (prefilter != 0)
    {
        weight /= 1.0f + dot(average, kLumaWeight);
    }
    return float4(average * weight, weight);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= dstSize.x || pixel.y >= dstSize.y)
    {
        return;
    }

    float2 uv = (float2(pixel) + 0.5f) / float2(dstSize);
    float2 t = srcTexelSize;

    // 13点を引き、重なり合う5つの四角に分けて平均する。
    // 単純な4点平均より、細い光が動いたときの明滅が出にくい
    float3 a = Fetch(uv + t * float2(-2.0f, -2.0f));
    float3 b = Fetch(uv + t * float2(0.0f, -2.0f));
    float3 c = Fetch(uv + t * float2(2.0f, -2.0f));
    float3 d = Fetch(uv + t * float2(-2.0f, 0.0f));
    float3 e = Fetch(uv);
    float3 f = Fetch(uv + t * float2(2.0f, 0.0f));
    float3 g = Fetch(uv + t * float2(-2.0f, 2.0f));
    float3 h = Fetch(uv + t * float2(0.0f, 2.0f));
    float3 i = Fetch(uv + t * float2(2.0f, 2.0f));
    float3 j = Fetch(uv + t * float2(-1.0f, -1.0f));
    float3 k = Fetch(uv + t * float2(1.0f, -1.0f));
    float3 l = Fetch(uv + t * float2(-1.0f, 1.0f));
    float3 m = Fetch(uv + t * float2(1.0f, 1.0f));

    float4 sum = WeightedBox(j, k, l, m, 0.5f);
    sum += WeightedBox(a, b, d, e, 0.125f);
    sum += WeightedBox(b, c, e, f, 0.125f);
    sum += WeightedBox(d, e, g, h, 0.125f);
    sum += WeightedBox(e, f, h, i, 0.125f);

    gBloomOutput[pixel] = float4(sum.rgb / max(sum.a, 1e-5f), 1.0f);
}
