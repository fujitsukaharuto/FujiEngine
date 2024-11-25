// 定数バッファ
struct LightningParams
{
    float2 rangeMin; // 描画範囲（最小UV）
    float2 rangeMax; // 描画範囲（最大UV）
    float2 resolution; // 画面解像度
    float time; // アニメーション時間
    float mainBranchStrength; // 主幹の強度
    float branchCount; // 分岐の数
    float branchFade; // 分岐のフェード率
    float highlightStrength; // ハイライトの強度
    float noiseScale; // ノイズのスケール
    float noiseSpeed; // ノイズの移動速度
};

Texture2D g_InputTexture : register(t0); // 元の画面テクスチャ
Texture2D noiseTexture : register(t1); // ノイズテクスチャ
SamplerState g_Sampler : register(s0);
ConstantBuffer<LightningParams> g_Lightning : register(b0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// 雷の主幹の形状を生成
float LightningMainBranch(float2 uv, float time)
{
    // 中心線のノイズによる揺らぎ
    float2 noiseUV = uv * g_Lightning.noiseScale + float2(0.0f, -time * g_Lightning.noiseSpeed);
    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

    // 主幹の位置を決定
    float centerLine = abs(uv.x - 0.5f + (noiseValue - 0.5f) * 0.1f); // 中心線の揺らぎ

    // 主幹の強度（細さ調整）
    return exp(-centerLine * g_Lightning.mainBranchStrength);
}

// 分岐の形状を生成
float LightningBranch(float2 uv, float time, float branchIndex)
{
    // 分岐の揺らぎを加える
    float offset = sin(branchIndex * 10.0f + time) * 0.1f;
    float2 branchUV = uv + float2(offset, branchIndex * 0.2f);

    // ノイズによる揺らぎ
    float2 noiseUV = branchUV * g_Lightning.noiseScale;
    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

    // 分岐の位置と強度
    float branchCenter = abs(branchUV.x - 0.5f + (noiseValue - 0.5f) * 0.2f);
    float branchStrength = exp(-branchCenter * g_Lightning.mainBranchStrength);

    // 分岐のフェード
    return branchStrength * exp(-branchUV.y * g_Lightning.branchFade);
}

float4 main(PSInput input) : SV_TARGET
{
    // 元の画面テクスチャを取得
    float4 baseColor = g_InputTexture.Sample(g_Sampler, input.texcoord);

    // 描画範囲を正規化
    float2 uv = (input.texcoord - g_Lightning.rangeMin.xy) / (g_Lightning.rangeMax.xy - g_Lightning.rangeMin.xy);

    // 範囲外ならそのまま返す
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return baseColor;
    }

    // 主幹の雷
    float mainBranch = LightningMainBranch(uv, g_Lightning.time);

    // 複数の分岐を追加
    float branchEffect = 0.0f;
    for (int i = 0; i < int(g_Lightning.branchCount); ++i)
    {
        branchEffect += LightningBranch(uv, g_Lightning.time, float(i) / g_Lightning.branchCount);
    }

    // 全体の雷強度
    float lightningIntensity = saturate(mainBranch + branchEffect);

    // ハイライトを加える
    lightningIntensity = pow(lightningIntensity, g_Lightning.highlightStrength);

    // 雷の色（白～青）
    float3 lightningColor = lerp(float3(0.2f, 0.4f, 1.0f), float3(1.0f, 1.0f, 1.0f), lightningIntensity);

    // 元のテクスチャとブレンド
    float3 finalColor = lerp(baseColor.rgb, lightningColor, lightningIntensity);

    // アルファ1.0で返す
    return float4(finalColor, 1.0f);
}