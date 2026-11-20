#include "../Particle/GPU/Splat.hlsli"

// GPU 粒子の蓄積を、映り込みが次のフレームに引く画面の写しへ足す (SplatComposite.PS と同じ色)

StructuredBuffer<uint> gSplatAccum : register(t0);
ConstantBuffer<SplatParam> gSplatParam : register(b0);

RWTexture2D<float4> gReflectionSceneColorOut : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (any(pixel >= gSplatParam.dims))
    {
        return;
    }
    uint base = SplatPixelBase(pixel.x, pixel.y, gSplatParam.dims.x);
    float3 splat = float3(
        (float) gSplatAccum[base + 0],
        (float) gSplatAccum[base + 1],
        (float) gSplatAccum[base + 2]) / (float) kSplatScale * gSplatParam.intensity;
    if (!any(splat > 0.0f))
    {
        return;
    }
    float4 color = gReflectionSceneColorOut[pixel];
    gReflectionSceneColorOut[pixel] = float4(color.rgb + splat, color.a);
}
