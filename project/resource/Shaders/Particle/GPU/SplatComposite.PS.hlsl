#include "Splat.hlsli"

// 蓄積バッファ(固定小数)を読み出して色に戻し、加算ブレンドでシーンへ重ねる。
StructuredBuffer<uint> gSplatAccum : register(t0);
ConstantBuffer<SplatParam> gSplatParam : register(b0);

struct PSInput
{
    float4 position : SV_POSITION;
};

float4 main(PSInput input) : SV_TARGET
{
    uint px = (uint) input.position.x;
    uint py = (uint) input.position.y;
    uint base = SplatPixelBase(px, py, gSplatParam.dims.x);

    float3 c = float3(
        (float) gSplatAccum[base + 0],
        (float) gSplatAccum[base + 1],
        (float) gSplatAccum[base + 2]) / (float) kSplatScale;

    // 加算合成(Src ONE / Dest ONE)前提なのでrgbをそのまま出す
    return float4(c, 1.0f);
}
