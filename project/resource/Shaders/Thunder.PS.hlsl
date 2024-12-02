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

//// 雷の主幹の形状を生成
//float LightningMainBranch(float2 uv, float time)
//{
//    // 中心線のノイズによる揺らぎ
//    float2 noiseUV = uv * g_Lightning.noiseScale + float2(0.0f, -time * g_Lightning.noiseSpeed);
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 主幹の位置を決定
//    float centerLine = abs(uv.x - 0.5f + (noiseValue - 0.5f) * 0.1f); // 中心線の揺らぎ

//    // 主幹の強度（細さ調整）
//    return exp(-centerLine * g_Lightning.mainBranchStrength);
//}

//// 分岐の形状を生成
//float LightningBranch(float2 uv, float time, float branchIndex)
//{
//    // 分岐の揺らぎを加える
//    float offset = sin(branchIndex * 10.0f + time) * 0.1f;
//    float2 branchUV = uv + float2(offset, branchIndex * 0.2f);

//    // ノイズによる揺らぎ
//    float2 noiseUV = branchUV * g_Lightning.noiseScale;
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 分岐の位置と強度
//    float branchCenter = abs(branchUV.x - 0.5f + (noiseValue - 0.5f) * 0.2f);
//    float branchStrength = exp(-branchCenter * g_Lightning.mainBranchStrength);

//    // 分岐のフェード
//    return branchStrength * exp(-branchUV.y * g_Lightning.branchFade);
//}

//float4 main(PSInput input) : SV_TARGET
//{
//    // 元の画面テクスチャを取得
//    float4 baseColor = g_InputTexture.Sample(g_Sampler, input.texcoord);

//    // 描画範囲を正規化
//    float2 uv = (input.texcoord - g_Lightning.rangeMin.xy) / (g_Lightning.rangeMax.xy - g_Lightning.rangeMin.xy);

//    // 範囲外ならそのまま返す
//    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
//    {
//        return baseColor;
//    }

//    // 主幹の雷
//    float mainBranch = LightningMainBranch(uv, g_Lightning.time);

//    // 複数の分岐を追加
//    float branchEffect = 0.0f;
//    for (int i = 0; i < int(g_Lightning.branchCount); ++i)
//    {
//        branchEffect += LightningBranch(uv, g_Lightning.time, float(i) / g_Lightning.branchCount);
//    }

//    // 全体の雷強度
//    float lightningIntensity = saturate(mainBranch + branchEffect);

//    // ハイライトを加える
//    lightningIntensity = pow(lightningIntensity, g_Lightning.highlightStrength);

//    // 雷の色（白～青）
//    float3 lightningColor = lerp(float3(0.2f, 0.4f, 1.0f), float3(1.0f, 1.0f, 1.0f), lightningIntensity);

//    // 元のテクスチャとブレンド
//    float3 finalColor = lerp(baseColor.rgb, lightningColor, lightningIntensity);

//    // アルファ1.0で返す
//    return float4(finalColor, 1.0f);
//}








//// スパーク
//// 雷の主幹の形状を生成
//float LightningMainBranch(float2 uv, float time)
//{
//    // 中心線のノイズによる揺らぎ
//    float2 noiseUV = uv * g_Lightning.noiseScale + float2(0.0f, -time * g_Lightning.noiseSpeed);
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 主幹の位置を決定
//    float centerLine = abs(uv.x - 0.5f + (noiseValue - 0.5f) * 0.1f); // 中心線の揺らぎ

//    // 主幹の強度（細さ調整）
//    return exp(-centerLine * g_Lightning.mainBranchStrength);
//}

//// 分岐の形状を生成
//float LightningBranch(float2 uv, float time, float branchIndex)
//{
//    // 分岐の揺らぎを加える
//    float offset = sin(branchIndex * 10.0f + time) * 0.1f;
//    float2 branchUV = uv + float2(offset, branchIndex * 0.2f);

//    // ノイズによる揺らぎ
//    float2 noiseUV = branchUV * g_Lightning.noiseScale;
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 分岐の位置と強度
//    float branchCenter = abs(branchUV.x - 0.5f + (noiseValue - 0.5f) * 0.2f);
//    float branchStrength = exp(-branchCenter * g_Lightning.mainBranchStrength);

//    // 分岐のフェード
//    return branchStrength * exp(-branchUV.y * g_Lightning.branchFade);
//}

//// ランダム方向に複数の雷を生成
//float LightningSpark(float2 uv, float time, float2 direction, float branchOffset)
//{
//    // 雷の方向に沿った位置を計算
//    float2 sparkUV = uv - 0.5f; // 中心基準
//    sparkUV = float2(dot(sparkUV, direction), length(sparkUV)); // ローカル座標変換

//    // ノイズによる揺らぎ
//    float2 noiseUV = sparkUV * g_Lightning.noiseScale + float2(branchOffset, -time * g_Lightning.noiseSpeed);
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 主幹の中心線を計算
//    float centerLine = abs(sparkUV.x + (noiseValue - 0.5f) * 0.2f);

//    // 強度計算（距離で減衰）
//    float strength = exp(-centerLine * g_Lightning.mainBranchStrength) * exp(-sparkUV.y * g_Lightning.branchFade);
//    return strength;
//}

//float4 main(PSInput input) : SV_TARGET
//{
//    // 元の画面テクスチャを取得
//    float4 baseColor = g_InputTexture.Sample(g_Sampler, input.texcoord);

//    // 描画範囲を正規化
//    float2 uv = (input.texcoord - g_Lightning.rangeMin.xy) / (g_Lightning.rangeMax.xy - g_Lightning.rangeMin.xy);

//    // 範囲外ならそのまま返す
//    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
//    {
//        return baseColor;
//    }

//    // 雷のスパーク効果
//    float lightningEffect = 0.0f;

//    // ランダムに方向を生成して複数の雷を描画
//    for (int i = 0; i < int(g_Lightning.branchCount); ++i)
//    {
//        float angle = (float(i) / g_Lightning.branchCount) * 6.28318f; // 円周方向
//        float2 direction = float2(cos(angle), sin(angle));
//        lightningEffect += LightningSpark(uv, g_Lightning.time, direction, float(i) * 0.1f);
//    }

//    // ハイライトを加える
//    lightningEffect = pow(saturate(lightningEffect), g_Lightning.highlightStrength);

//    // 雷の色（白～青）
//    float3 lightningColor = lerp(float3(0.2f, 0.4f, 1.0f), float3(1.0f, 1.0f, 1.0f), lightningEffect);

//    // 元のテクスチャとブレンド
//    float3 finalColor = lerp(baseColor.rgb, lightningColor, lightningEffect);

//    // アルファ1.0で返す
//    return float4(finalColor, 1.0f);
//}











//// 雷の進行方向を下方向に修正
//float LightningMainBranch(float2 uv, float time)
//{
//    // 中心線のノイズによる揺らぎ（進行方向を下に設定）
//    float2 noiseUV = uv * g_Lightning.noiseScale + float2(0.0f, -time * g_Lightning.noiseSpeed);
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 主幹の位置を決定
//    float centerLine = abs(uv.x - 0.5f + (noiseValue - 0.5f) * 0.1f); // 中心線の揺らぎ

//    // 主幹の強度（細さ調整）
//    return exp(-centerLine * g_Lightning.mainBranchStrength);
//}

//// 地面に広がる分岐を追加
//float LightningBranch(float2 uv, float time, float branchIndex)
//{
//    // 分岐の揺らぎを加える（地面に向かって広がるように調整）
//    float offset = sin(branchIndex * 10.0f + time) * 0.1f;
//    float2 branchUV = uv + float2(offset, branchIndex * 0.3f); // 地面に向かって伸びる

//    // ノイズによる揺らぎ
//    float2 noiseUV = branchUV * g_Lightning.noiseScale;
//    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

//    // 分岐の位置と強度
//    float branchCenter = abs(branchUV.x - 0.5f + (noiseValue - 0.5f) * 0.2f);
//    float branchStrength = exp(-branchCenter * g_Lightning.mainBranchStrength);

//    // 分岐のフェード（地面付近で強調）
//    return branchStrength * exp(-branchUV.y * g_Lightning.branchFade);
//}

//float4 main(PSInput input) : SV_TARGET
//{
//    // 元の画面テクスチャを取得
//    float4 baseColor = g_InputTexture.Sample(g_Sampler, input.texcoord);

//    // 描画範囲を正規化
//    float2 uv = (input.texcoord - g_Lightning.rangeMin.xy) / (g_Lightning.rangeMax.xy - g_Lightning.rangeMin.xy);

//    // 範囲外ならそのまま返す
//    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
//    {
//        return baseColor;
//    }

//    // 主幹の雷
//    float mainBranch = LightningMainBranch(uv, g_Lightning.time);

//    // 複数の分岐を追加（地面に向かって広がるように）
//    float branchEffect = 0.0f;
//    float branchWeight = 1.0f / g_Lightning.branchCount; // 各分岐の影響を均等化

//    for (int i = 0; i < int(g_Lightning.branchCount); ++i)
//    {
//        float branchIndex = float(i) / g_Lightning.branchCount;
//        branchEffect += LightningBranch(uv, g_Lightning.time, branchIndex) * branchWeight;
//    }

//    // 主幹と分岐の合成
//    float lightningIntensity = saturate(mainBranch + branchEffect);

//    // ハイライトを加える
//    lightningIntensity = pow(lightningIntensity, g_Lightning.highlightStrength);

//    // 雷の色（白～青）
//    float3 lightningColor = lerp(float3(0.2f, 0.4f, 1.0f), float3(1.0f, 1.0f, 1.0f), lightningIntensity);

//    // 元のテクスチャとブレンド
//    float3 finalColor = lerp(baseColor.rgb, lightningColor, lightningIntensity);

//    // アルファ1.0で返す
//    return float4(finalColor, 1.0f);
//}











// 雷の進行方向を下方向に修正
float LightningMainBranch(float2 uv, float time)
{
    // 中心線のノイズによる揺らぎ（進行方向を下に設定）
    float2 noiseUV = uv * g_Lightning.noiseScale + float2(0.0f, -time * g_Lightning.noiseSpeed);
    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

    // 主幹の位置を決定
    float centerLine = abs(uv.x - 0.5f + (noiseValue - 0.5f) * 0.1f); // 中心線の揺らぎ

    // 主幹の強度（細さ調整）
    return exp(-centerLine * g_Lightning.mainBranchStrength);
}

// 地面に広がる分岐を追加
float LightningBranch(float2 uv, float time, float branchIndex)
{
    // 分岐の揺らぎを加える（地面に向かって広がるように調整）
    float offset = sin(branchIndex * 10.0f + time) * 0.1f;
    float2 branchUV = uv + float2(offset, branchIndex * 0.3f); // 地面に向かって伸びる

    // ノイズによる揺らぎ
    float2 noiseUV = branchUV * g_Lightning.noiseScale;
    float noiseValue = noiseTexture.Sample(g_Sampler, noiseUV).r;

    // 分岐の位置と強度
    float branchCenter = abs(branchUV.x - 0.5f + (noiseValue - 0.5f) * 0.2f);
    float branchStrength = exp(-branchCenter * g_Lightning.mainBranchStrength);

    // 分岐のフェード（地面付近で強調）
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

    // 複数の分岐を追加（地面に向かって広がるように）
    float branchEffect = 0.0f;
    float branchWeight = 1.0f / g_Lightning.branchCount; // 各分岐の影響を均等化

// 分岐が地面に向かって広がるように
    for (int i = 0; i < int(g_Lightning.branchCount); ++i)
    {
    // 各分岐の位置を地面に向けて広げる
        float branchIndex = float(i) / float(g_Lightning.branchCount);

    // 分岐の効果を計算
        float branch = LightningBranch(uv, g_Lightning.time, branchIndex);

    // 分岐の影響を加算
        branchEffect += branch * branchWeight;
    }

// 主幹と分岐の合成
    float lightningIntensity = saturate(mainBranch + branchEffect);

    // ハイライトを加える
    lightningIntensity = pow(lightningIntensity, g_Lightning.highlightStrength);

    // 雷の色（白～青）
    float3 lightningColor = lerp(float3(0.2f, 0.4f, 1.0f), float3(0.5f, 0.7f, 1.0f), lightningIntensity);

    // 元のテクスチャとブレンド
    float3 finalColor = lerp(baseColor.rgb, lightningColor, lightningIntensity);

    // アルファ1.0で返す
    return float4(finalColor, 1.0f);
}