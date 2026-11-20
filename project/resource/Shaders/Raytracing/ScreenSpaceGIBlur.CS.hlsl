#include "../Common/ScreenSpaceGICommon.hlsli"

// 同じ面の画素だけで横→縦に混ぜる。1段目でレイの向きを分け合った 4x4 の模様を消し、2段目で広くならす

Texture2D<float4> gSSGIBlurInput : register(t3, space13);
RWTexture2D<float4> gSSGIOut : register(u0); // rgb=照り返し, a=遮られた割合

// 1段目: 両端を半分にした5タップ。4画素周期の模様がちょうど消える
static const float kPatternWeights[5] = { 0.5f, 1.0f, 1.0f, 1.0f, 0.5f };
// 2段目: 2画素おきに片側3タップのガウス
static const int kWideHalfTaps = 3;
static const int kWideSpacing = 2;
// 自分の面からこれ以上離れたものは混ぜない。カメラからの距離に対する割合
static const float kPlaneTolerance = 0.02f;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (any(pixel >= screenSize))
    {
        return;
    }

    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gSSGIOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float3 position = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
    float tolerance = max(kPlaneTolerance * distance(position, cameraPos), 0.01f);

    int halfTaps = (blurWide != 0) ? kWideHalfTaps : 2;
    int spacing = (blurWide != 0) ? kWideSpacing : 1;
    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    for (int k = -halfTaps; k <= halfTaps; k++)
    {
        int2 tap = int2(pixel) + blurDirection * (k * spacing);
        float3 tapNormal;
        float tapDepth;
        if (any(tap < 0) || any(tap >= int2(screenSize)) || !LoadSurface(gGBufferNormal, gGBufferDepth, tap, tapNormal, tapDepth))
        {
            continue;
        }
        float3 tapPosition = ReconstructWorldPos(uint2(tap), screenSize, tapDepth, invViewProj);
        float t = float(k) / float(halfTaps);
        float weight = (blurWide != 0) ? exp(-2.0f * t * t) : kPatternWeights[k + 2];
        weight *= pow(saturate(dot(normal, tapNormal)), 8.0f);
        weight *= exp(-abs(dot(normal, tapPosition - position)) / tolerance);
        sum += gSSGIBlurInput.Load(int3(tap, 0)) * weight;
        weightSum += weight;
    }

    gSSGIOut[pixel] = sum / max(weightSum, 1e-4f);
}
