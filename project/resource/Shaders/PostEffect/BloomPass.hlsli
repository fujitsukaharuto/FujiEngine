#ifndef BLOOM_PASS_HLSLI
#define BLOOM_PASS_HLSLI

// 多段のブルーム。明るい所を半分ずつ縮めながらぼかし(BloomDownsample)、
// 小さい方から順に重ねながら戻し(BloomUpsample)、最後にシーンへ足す(BloomComposite)。
// 1枚を広くぼかすより安く、光源のすぐ周りから遠くまで滑らかに広がる

// BloomPass.h の BloomPassParam と並びが一致していること
cbuffer BloomPassParam : register(b0)
{
    float2 srcTexelSize; // 入力の1テクセルの大きさ(UV)
    uint2 dstSize;       // 出力の画素数
    float threshold;     // これより明るい所が光る
    float knee;          // しきい値の前後をどれだけなだらかに立ち上げるか
    float scatter;       // 広がり。大きいほど小さい段(広いぼけ)が強く残る
    float intensity;     // シーンへ足すときの強さ
    uint prefilter;      // 0以外なら明るい所の抽出もする(最初の縮小だけ)
    float3 bloomPad;
};

SamplerState gSampler : register(s0);

static const float3 kLumaWeight = float3(0.2126f, 0.7152f, 0.0722f);

/// <summary>端を越えないようにして引く</summary>
/// <remarks>既定のサンプラーは WRAP なので、そのまま引くと画面の反対側の光がにじんでくる</remarks>
float3 SampleClamped(Texture2D<float4> tex, float2 uv)
{
    uv = clamp(uv, 0.5f * srcTexelSize, 1.0f - 0.5f * srcTexelSize);
    return tex.SampleLevel(gSampler, uv, 0.0f).rgb;
}

/// <summary>3x3 のテント型で引く。拡大したときに画素の格子が見えないようにする</summary>
float3 SampleTent(Texture2D<float4> tex, float2 uv)
{
    float2 d = srcTexelSize;

    float3 sum = SampleClamped(tex, uv) * 4.0f;
    sum += SampleClamped(tex, uv + float2(-d.x, 0.0f)) * 2.0f;
    sum += SampleClamped(tex, uv + float2(d.x, 0.0f)) * 2.0f;
    sum += SampleClamped(tex, uv + float2(0.0f, -d.y)) * 2.0f;
    sum += SampleClamped(tex, uv + float2(0.0f, d.y)) * 2.0f;
    sum += SampleClamped(tex, uv + float2(-d.x, -d.y));
    sum += SampleClamped(tex, uv + float2(d.x, -d.y));
    sum += SampleClamped(tex, uv + float2(-d.x, d.y));
    sum += SampleClamped(tex, uv + float2(d.x, d.y));

    return sum / 16.0f;
}

#endif // BLOOM_PASS_HLSLI
