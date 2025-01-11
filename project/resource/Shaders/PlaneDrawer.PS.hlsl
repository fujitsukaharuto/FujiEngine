struct VertexInput
{
    float3 position : POSITION; // ワールド座標
    float2 uv : TEXCOORD0; // テクスチャ座標
};

struct PixelInput
{
    float4 position : SV_POSITION; // クリップ座標
    float2 uv : TEXCOORD0; // テクスチャ座標 (そのまま渡す)
};

float4 main(PixelInput input) : SV_TARGET
{
    // 簡易的なカラー出力 (テクスチャ未使用の場合)
    return float4(input.uv, 0.5f, 1.0f);
}