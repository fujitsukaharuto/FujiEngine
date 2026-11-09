#include "../Common/VolumetricLightCommon.hlsli"

// 光の筋の 4x4 の箱ぼかし。進み始めのずらしの模様がちょうど消える
// 奥行きの離れた画素は混ぜない

Texture2D<float4> gVolumetricBlurInput : register(t0);
RWTexture2D<float4> gVolumetricOut : register(u0);

// 奥行きの差がこの割合を超えた画素は混ぜない
static const float kDepthTolerance = 0.1f;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    int2 pixel = int2(dispatchID.xy);
    if (any(pixel >= int2(lowSize)))
    {
        return;
    }

    float4 center = gVolumetricBlurInput.Load(int3(pixel, 0));
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    for (int y = -2; y < 2; ++y)
    {
        for (int x = -2; x < 2; ++x)
        {
            int2 tap = clamp(pixel + int2(x, y), int2(0, 0), int2(lowSize) - 1);
            float4 value = gVolumetricBlurInput.Load(int3(tap, 0));
            float weight = exp(-abs(value.a - center.a) / (kDepthTolerance * center.a + 0.1f));
            sum += value.rgb * weight;
            weightSum += weight;
        }
    }
    gVolumetricOut[pixel] = float4(sum / max(weightSum, 1e-4f), center.a);
}
