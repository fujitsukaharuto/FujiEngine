// 抜きのある物の影を抜けた形にする (Common/RayTracedShadow.hlsli)
#define RAY_CUTOUT_SHADOW
#include "../Object/Object3d.hlsli"
#include "../Common/RayTracedShadow.hlsli"
#include "../Common/ScreenSpaceShadow.hlsli"
#include "../Common/GBufferUtil.hlsli"
#include "../Common/Random.hlsli"

// 柔らかい影の枠 (AllLights::softShadowLights) の光源ごとの遮蔽率を rgb へ書き出す

ConstantBuffer<AllLights> gLights : register(b0);

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b1)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// PunctualShadowPass.h の PunctualShadowParam と並びが一致していること
cbuffer PunctualShadowParam : register(b2)
{
    uint2 screenSize;
    float temporalJitter; // フレームごとにサンプルの向きをずらす量。[0,1)
    uint punctualPad;
};

Texture2D<float4> gGBufferNormal : register(t0);
Texture2D<float> gGBufferDepth : register(t1);

RWTexture2D<float4> gPunctualShadowOutput : register(u0);

// 枠ごとに散らし方をずらす量 (黄金比の共役)
static const float kSlotSeedOffset = 0.618034f;

/// <summary>枠1つぶんの遮蔽率。光の届かない画素は 1</summary>
float SlotShadow(uint slot, float3 worldPos, float3 N, float seed, float bias)
{
    uint key = (gLights.softShadowLights >> (slot * 8u)) & 0xFFu;
    if (key == 0xFFu)
    {
        return 1.0f;
    }
    uint index = key & ~kSoftShadowSlotSpot;
    float3 lightPos;
    float sourceRadius;
    if ((key & kSoftShadowSlotSpot) != 0)
    {
        SpotLight light = gLights.spotLights[index];
        float3 lightDirOnSurface = normalize(worldPos - light.position);
        if (SpotLightAttenuation(light, worldPos, lightDirOnSurface) <= 0.0f)
        {
            return 1.0f;
        }
        lightPos = light.position;
        sourceRadius = light.sourceRadius;
    }
    else
    {
        PointLight light = gLights.pointLights[index];
        if (PointLightAttenuation(light, worldPos) <= 0.0f)
        {
            return 1.0f;
        }
        lightPos = light.position;
        sourceRadius = light.sourceRadius;
    }

    // 光源の円板が丸ごと面の裏に沈んでいれば影
    float3 toLight = lightPos - worldPos;
    float distance = length(toLight);
    float sinCone = saturate(sourceRadius / max(distance, 1e-4f));
    if (dot(N, toLight / max(distance, 1e-4f)) <= -sinCone)
    {
        return 0.0f;
    }
    return TraceSoftShadowPunctual(worldPos, N, lightPos, sourceRadius, gLights.shadowSampleCount,
                                   frac(seed + kSlotSeedOffset * float(slot)), bias);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= screenSize.x || pixel.y >= screenSize.y)
    {
        return;
    }

    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gPunctualShadowOutput[pixel] = float4(1.0f, 1.0f, 1.0f, 0.0f);
        return;
    }

    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
    float seed = frac(rand3dTo1d(worldPos) + temporalJitter);
    float bias = ReconstructedRayBias(distance(worldPos, cameraPos));

    float3 shadow;
    shadow.r = SlotShadow(0, worldPos, normal, seed, bias);
    shadow.g = SlotShadow(1, worldPos, normal, seed, bias);
    shadow.b = SlotShadow(2, worldPos, normal, seed, bias);
    gPunctualShadowOutput[pixel] = float4(shadow, 0.0f);
}
