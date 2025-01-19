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
    // 対角線(0,0)から(1,1)でのグラデーション値
    float gradient = (input.uv.x + input.uv.y) * 0.5f;

    // グラデーション値を使って青から赤に変化
    float3 color = lerp(float3(0.0f, 0.0f, 1.0f), float3(1.0f, 0.0f, 0.0f), gradient);

    return float4(color, 1.0f);
}