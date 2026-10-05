#include "BloomPass.hlsli"

// ブルームの合成。積み上げたぼけを画面の大きさへ拡大して、元のシーンへ足す

Texture2D<float4> InputTexture : register(t0); // 元のシーン
Texture2D<float4> gBloomLow : register(t1);    // 積み上げたぼけ(画面の半分の大きさ)
RWTexture2D<float4> outputTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= dstSize.x || pixel.y >= dstSize.y)
    {
        return;
    }

    float2 uv = (float2(pixel) + 0.5f) / float2(dstSize);

    float3 scene = InputTexture.Load(int3(pixel, 0)).rgb;
    float3 bloom = SampleTent(gBloomLow, uv);

    outputTexture[pixel] = float4(scene + bloom * intensity, 1.0f);
}
