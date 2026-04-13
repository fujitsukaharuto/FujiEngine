struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint : TEXCOORD1;
};

// カメラの逆行列（ViewProjectionの逆行列）を受け取る
cbuffer CameraInfo : register(b0)
{
    matrix invViewProj;
    float3 cameraPos;
};

// クリップ空間からワールド空間への変換関数
float3 UnprojectPoint(float x, float y, float z, matrix invVP)
{
    float4 unprojectedPoint = mul(float4(x, y, z, 1.0), invVP);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

VSOutput main(uint id : SV_VertexID)
{
    VSOutput output;

    // id (0, 1, 2) から画面全体を覆う三角形のUV(0~2)と座標(-1~3)を生成
    float2 uv = float2((id << 1) & 2, id & 2);
    float2 clipPos = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    output.pos = float4(clipPos, 0.0, 1.0);

    // このピクセルに向かう視線の始点（Near）と終点（Far）のワールド座標を計算
    output.nearPoint = UnprojectPoint(clipPos.x, clipPos.y, 0.0, invViewProj);
    output.farPoint = UnprojectPoint(clipPos.x, clipPos.y, 1.0, invViewProj);

    return output;
}