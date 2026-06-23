// 頂点バッファ不要のフルスクリーン三角形(SV_VertexID生成)
struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput main(uint id : SV_VertexID)
{
    PSInput output;
    // id=0:(0,0) 1:(2,0) 2:(0,2) のUVから画面を覆う大きな三角形を作る
    float2 uv = float2((id << 1) & 2, id & 2);
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
