struct VS_INPUT
{
    float4 pos : POSITION; // 頂点位置
    float2 uv : TEXCOORD; // UV座標
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION; // スクリーン空間座標
    float2 uv : TEXCOORD0; // UV座標
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos); // 画面座標をそのまま変換
    output.uv = input.uv; // UV座標をそのまま渡す
    return output;
}