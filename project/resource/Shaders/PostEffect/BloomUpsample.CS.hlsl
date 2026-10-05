#include "BloomPass.hlsli"

// ブルームの拡大。1段小さい方を拡大して、同じ大きさの縮小結果と混ぜる。
// 足し合わせずに混ぜるので、段数を変えても全体の明るさは変わらない

Texture2D<float4> gBloomLow : register(t0);  // 1段小さい方。ここまで積み上げてきた広いぼけ
Texture2D<float4> gBloomHigh : register(t1); // 出力と同じ大きさの縮小結果。細かいぼけ
RWTexture2D<float4> gBloomOutput : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= dstSize.x || pixel.y >= dstSize.y)
    {
        return;
    }

    float2 uv = (float2(pixel) + 0.5f) / float2(dstSize);

    float3 low = SampleTent(gBloomLow, uv);
    float3 high = gBloomHigh.Load(int3(pixel, 0)).rgb;

    gBloomOutput[pixel] = float4(lerp(high, low, scatter), 1.0f);
}
