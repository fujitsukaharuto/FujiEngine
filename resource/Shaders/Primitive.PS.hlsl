
// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct VSOutput
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 color : COLOR; // 色(RGBA)
};

float4 main(VSOutput input) : SV_TARGET{
    return input.color;
}