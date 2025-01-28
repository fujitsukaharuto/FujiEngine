struct Camera
{
    matrix viewProject;
};
ConstantBuffer<Camera> gCamera : register(b0);

struct VertexInput
{
    float3 position : POSITION; // ワールド座標
    float2 uv : TEXCOORD0; // テクスチャ座標
    float4 color : COLOR;
};

struct PixelInput
{
    float4 position : SV_POSITION; // クリップ座標
    float2 uv : TEXCOORD0; // テクスチャ座標 (そのまま渡す)
    float4 color : COLOR;
};

PixelInput main(VertexInput input)
{
    PixelInput output;

    // ワールド座標をビュー・プロジェクション行列で変換
    output.position = mul(float4(input.position,1.0f), gCamera.viewProject);

    // テクスチャ座標をそのまま渡す
    output.uv = input.uv;

    output.color = input.color;
    
    return output;
}