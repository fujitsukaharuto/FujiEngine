#ifndef SCREEN_SPACE_GI_COMMON_HLSLI
#define SCREEN_SPACE_GI_COMMON_HLSLI

// 画面の照り返し (SSGI) の CS 側の共通部分
#include "GBufferUtil.hlsli"

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b1)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// ScreenSpaceGIPass.h の SSGIPassParam と並びが一致していること
cbuffer SSGIPassParam : register(b2)
{
    float4x4 sceneColorViewProj; // 写した画面 (前のフレーム) の ViewProjection
    uint2 screenSize;
    float maxDistance;           // レイの届く距離 (m)
    float intensity;             // 照り返しに掛ける倍率
    int2 blurDirection;          // (1,0)で横、(0,1)で縦
    uint useSceneColor;          // 0 なら写した画面が無い
    uint blurWide;               // 0 なら模様を消す5タップ、1 なら広いガウス
    float2 depthToViewZ;         // 奥行き = x / (深度 - y)
    float2 ssgiPad;
};

Texture2D<float4> gGBufferNormal : register(t0, space13);
Texture2D<float> gGBufferDepth : register(t1, space13);

#endif // SCREEN_SPACE_GI_COMMON_HLSLI
