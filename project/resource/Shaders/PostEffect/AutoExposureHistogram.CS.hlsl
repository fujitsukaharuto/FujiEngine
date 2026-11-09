#include "AutoExposure.hlsli"

// 画面の明るさを対数のヒストグラムへ数える。2x2 画素に1つ、真ん中ほど重く (中央重点測光)

Texture2D<float4> InputTexture : register(t0);
RWStructuredBuffer<uint> gHistogram : register(u0);

static const float3 kLumaWeight = float3(0.2126f, 0.7152f, 0.0722f);

groupshared uint gsBins[kAutoExposureBinCount];

[numthreads(16, 16, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID, uint threadIndex : SV_GroupIndex)
{
    if (threadIndex < kAutoExposureBinCount)
    {
        gsBins[threadIndex] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint width;
    uint height;
    InputTexture.GetDimensions(width, height);
    uint2 pixel = dispatchID.xy * 2;
    if (all(pixel < uint2(width, height)))
    {
        float luminance = dot(InputTexture[pixel].rgb, kLumaWeight);
        float logLuminance = log2(max(luminance, 1e-8f));
        float t = saturate((logLuminance - kAutoExposureMinLog) / (kAutoExposureMaxLog - kAutoExposureMinLog));
        uint bin = min(uint(t * kAutoExposureBinCount), kAutoExposureBinCount - 1);

        // 中央が 4、四隅が 1 の重み
        float2 uv = (float2(pixel) + 0.5f) / float2(width, height);
        float edge = saturate(length(uv - 0.5f) * 1.41421356f);
        uint weight = uint(lerp(4.0f, 1.0f, edge) + 0.5f);
        InterlockedAdd(gsBins[bin], weight);
    }
    GroupMemoryBarrierWithGroupSync();

    if (threadIndex < kAutoExposureBinCount && gsBins[threadIndex] != 0)
    {
        InterlockedAdd(gHistogram[threadIndex], gsBins[threadIndex]);
    }
}
