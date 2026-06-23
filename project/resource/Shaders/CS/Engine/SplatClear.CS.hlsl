#include "Splat.hlsli"

// 蓄積バッファを0クリアする
RWStructuredBuffer<uint> gSplatAccum : register(u0);
ConstantBuffer<SplatParam> gSplatParam : register(b0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint total = gSplatParam.dims.x * gSplatParam.dims.y * 4;
    if (DTid.x < total)
    {
        gSplatAccum[DTid.x] = 0;
    }
}
