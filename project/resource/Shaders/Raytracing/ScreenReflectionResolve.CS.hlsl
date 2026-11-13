#include "../Common/ScreenReflectionCommon.hlsli"

// 鏡の向きのレイの結果を、粗さと当たった距離に応じて横→縦のガウスでぼかす。同じ面で粗さの近い画素だけ混ぜる

Texture2D<float4> gReflectionTrace : register(t4, space8);     // レイの結果。ぼかす広さは中心の距離で決める
Texture2D<float4> gReflectionBlurInput : register(t5, space8); // ぼかす元。1回目はレイの結果、2回目は1回目の出力
// rgb=当たった物の明るさ, a=0.5+空が見えた割合/2 (0なら使えない)
RWTexture2D<float4> gReflectionOut : register(u0);

// 粗さから円錐の広がり (tan)
static const float kConeScale = 2.0f;
// ぼかす半径の上限 (画素)。タップを1画素おきに並べられる広さまで
static const float kMaxRadius = 6.0f;
static const int kHalfTaps = 6;
// 縦に伸ばすときの倍率の上限と、そのときの片側のタップ数の上限
static const float kMaxStretch = 6.0f;
static const int kMaxStretchHalfTaps = 18;
// 自分の面からこれ以上離れたものは混ぜない。カメラからの距離に対する割合
static const float kPlaneTolerance = 0.02f;

/// <summary>ぼかす元の1画素を、当たった物の明るさと空の割合に直す</summary>
/// <returns>使える画素か</returns>
bool DecodeBlurInput(float4 value, out float3 radiance, out float sky)
{
    if (inputIsTrace != 0)
    {
        // a=当たった距離、空なら -1、飛ばしていなければ 0
        radiance = value.a < 0.0f ? float3(0.0f, 0.0f, 0.0f) : value.rgb;
        sky = value.a < 0.0f ? 1.0f : 0.0f;
        return value.a != 0.0f;
    }
    radiance = value.rgb;
    sky = saturate(value.a * 2.0f - 1.0f);
    return value.a > 0.0f;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (any(pixel >= screenSize))
    {
        return;
    }

    float4 trace = gReflectionTrace.Load(int3(pixel, 0));
    float3 radiance;
    float sky;
    float3 normal;
    float depth;
    if (trace.a == 0.0f || !DecodeBlurInput(gReflectionBlurInput.Load(int3(pixel, 0)), radiance, sky) ||
        !LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gReflectionOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float roughness;
    bool isExplicit;
    DecodeReflectionRoughness(gGBufferNormal.Load(int3(pixel, 0)).a, roughness, isExplicit);

    float3 position = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
    float viewDistance = distance(position, cameraPos);
    // 像の上での円錐の広がりを画素数に直す。2回とも中心から求める
    float hitDistance = trace.a < 0.0f ? maxRayDistance : trace.a;
    float coneTan = kConeScale * roughness * roughness;
    // 上限は粗さにも比例させる
    float maxRadius = kMaxRadius * saturate(roughness / max(maxRoughness, 1e-3f));
    float radius = min(coneTan * hitDistance / (viewDistance + hitDistance) * pixelsPerUnit, maxRadius);
    // 浅い角度ほど縦に伸ばす (縦のぼかしだけ)
    int maxHalfTaps = kHalfTaps;
    if (stretch > 0.0f && blurDirection.y != 0)
    {
        float NdotV = saturate(dot(normal, normalize(cameraPos - position)));
        float factor = min(1.0f + stretch * (1.0f / max(NdotV, 0.05f) - 1.0f), kMaxStretch);
        radius *= factor;
        maxHalfTaps = kMaxStretchHalfTaps;
    }

    float3 sum = radiance;
    float skySum = sky;
    float weightSum = 1.0f;
    if (radius >= 0.75f)
    {
        float tolerance = max(kPlaneTolerance * viewDistance, 0.01f);
        // タップが重ならず間も空かない本数
        int halfTaps = clamp(int(ceil(radius)), 1, maxHalfTaps);
        float spacing = radius / float(halfTaps);
        for (int k = -halfTaps; k <= halfTaps; k++)
        {
            if (k == 0)
            {
                continue;
            }
            int2 tap = int2(pixel) + blurDirection * int(round(float(k) * spacing));
            if (any(tap < 0) || any(tap >= int2(screenSize)) || all(tap == int2(pixel)))
            {
                continue;
            }
            float3 tapRadiance;
            float tapSky;
            float3 tapNormal;
            float tapDepth;
            if (!DecodeBlurInput(gReflectionBlurInput.Load(int3(tap, 0)), tapRadiance, tapSky) ||
                !LoadSurface(gGBufferNormal, gGBufferDepth, tap, tapNormal, tapDepth))
            {
                continue;
            }
            float tapRoughness;
            bool tapExplicit;
            DecodeReflectionRoughness(gGBufferNormal.Load(int3(tap, 0)).a, tapRoughness, tapExplicit);
            float3 tapPosition = ReconstructWorldPos(uint2(tap), screenSize, tapDepth, invViewProj);

            // 半径で標準偏差の2倍になるガウス
            float t = float(k) / float(halfTaps);
            float weight = exp(-2.0f * t * t);
            weight *= pow(saturate(dot(normal, tapNormal)), 16.0f);
            weight *= exp(-abs(dot(normal, tapPosition - position)) / tolerance);
            weight *= exp(-abs(tapRoughness - roughness) * 8.0f);
            sum += tapRadiance * weight;
            skySum += tapSky * weight;
            weightSum += weight;
        }
    }

    gReflectionOut[pixel] = float4(sum / weightSum, 0.5f + 0.5f * saturate(skySum / weightSum));
}
