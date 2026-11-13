#ifndef SCREEN_REFLECTION_COMMON_HLSLI
#define SCREEN_REFLECTION_COMMON_HLSLI

// 画面の映り込みの CS 側の共通部分。画素ごとに鏡の向きへ1本飛ばし、粗さと距離から画面の上でぼかす
#include "GBufferUtil.hlsli"

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b1)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// ScreenReflectionPass.h の ReflectionPassParam と並びが一致していること
cbuffer ReflectionPassParam : register(b2)
{
    uint2 screenSize;
    uint reflectAll;      // 0以外なら映り込みを付けていないマテリアルからも飛ばす
    float maxRoughness;   // これより粗い面からは飛ばさない
    float maxRayDistance; // レイの届く距離。空の像はここにあるものとしてぼかす
    float pixelsPerUnit;  // 距離1で長さ1が画面上で何画素か (縦)
    float2 reflectionPad;
    int2 blurDirection;   // ぼかす向き。(1,0)で横、(0,1)で縦
    uint inputIsTrace;    // 0以外ならぼかす元がレイの結果
    float stretch;        // 縦のぼかしで、浅い角度ほど伸ばす強さ
    float4x4 sceneColorViewProj; // 写した画面 (前のフレーム) の ViewProjection
    uint useSceneColor;          // 0以外なら写した画面の色を使う
    float3 sceneColorPad;
};

// space7 は使用済みなので space8
Texture2D<float4> gGBufferNormal : register(t0, space8);
Texture2D<float> gGBufferDepth : register(t1, space8);

/// <summary>プリパスが法線の a に入れた粗さを戻す。reflectAll で付けた物は2を足してある</summary>
/// <param name="isExplicit">マテリアルが映り込みを付けているか</param>
/// <returns>飛ばしてよい粗さか</returns>
bool DecodeReflectionRoughness(float encoded, out float roughness, out bool isExplicit)
{
    isExplicit = encoded < 2.0f;
    roughness = isExplicit ? encoded : encoded - 2.0f;
    return (isExplicit || reflectAll != 0) && roughness <= maxRoughness;
}

#endif // SCREEN_REFLECTION_COMMON_HLSLI
