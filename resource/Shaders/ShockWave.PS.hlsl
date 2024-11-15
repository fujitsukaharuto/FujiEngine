// 定数バッファ（C++側から渡されるパラメータ）
struct ShockwaveParams
{
    float4 shockCenter; // 衝撃波の中心（スクリーン空間座標）
    float shockTime; // 時間（衝撃波の広がりを制御）
    float shockRadius; // 衝撃波の最大半径
    float shockIntensity; // 歪みの強度
};

// シェーダーリソース
Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);
ConstantBuffer<ShockwaveParams> g_Shock : register(b0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};


// ピクセルシェーダー
float4 main(PSInput input) : SV_TARGET
{
    // 衝撃波の中心からの距離を計算
    float2 direction = input.texcoord - g_Shock.shockCenter.xy;
    float distance = length(direction);

    // 衝撃波の到達範囲を計算
    float currentRadius = g_Shock.shockRadius * g_Shock.shockTime;

    // 衝撃波の範囲チェックと減衰計算
    if (distance < currentRadius && distance > (currentRadius - 0.1f))
    {
        // 距離に基づき、サイン波で周期的な歪みを与える
        float distortion = sin((currentRadius - distance) * 20.0f) * g_Shock.shockIntensity * (1.0f - g_Shock.shockTime);

        // サンプリング位置のオフセットを計算
        float2 distortedTexCoord = input.texcoord + normalize(direction) * distortion;

        // UV 座標を [0, 1] の範囲にクランプ
        distortedTexCoord = clamp(distortedTexCoord, 0.0f, 1.0f);

        // 歪みを適用したテクスチャ座標でテクスチャをサンプリング
        return g_InputTexture.Sample(g_Sampler, distortedTexCoord);
    }
    else
    {
        // 衝撃波の範囲外では通常のテクスチャサンプリング
        return g_InputTexture.Sample(g_Sampler, input.texcoord);
    }
}