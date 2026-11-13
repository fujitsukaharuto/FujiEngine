#ifndef RAIN_HLSLI
#define RAIN_HLSLI

// 雨 (RainRenderer)。雨粒の番号からカメラのまわりの箱の中の位置を決める。位置は世界に固定

// RainRenderer.h の RainParams と並びが一致していること
cbuffer RainParams : register(b0)
{
    float4x4 rainViewProjection;
    float3 rainCameraPosition;
    float rainTime;          // 秒
    float3 rainAmbient;      // 空からの明るさ
    float rainBoxSize;       // カメラのまわりの箱の1辺 (m)
    float2 rainWind;         // 横に流れる速さ (m/s、xz)
    float rainFallSpeed;     // 落ちる速さ (m/s)
    float rainStreakLength;  // 雨粒の筋の長さ (m)
    float rainWidth;         // 筋の太さ (m)
    float rainBrightness;    // 照らされた雨粒の明るさの倍率
    float2 rainScreenSize;   // 描画の解像度
    float2 splashCenter;     // しぶきを散らす範囲の中心 (xz)
    float splashArea;        // しぶきを散らす範囲の1辺 (m)
    float splashBrightness;  // しぶきの明るさの倍率
    float4x4 rainInverseViewProjection; // 遠くの雨のカーテンで画素の向きを戻す
    int2 dripGridBase;       // 雨だれの格子のいちばん小さいマス
    float dripCellSize;      // 雨だれの格子の1マス (m)
    uint dripFrame;          // 雨だれの場所を少しずつ探し直す番号
    float dripBrightness;    // 雨だれの明るさの倍率
    float dripMaxFall;       // これより高い所からは垂らさない (m)
    float curtainBrightness; // 遠くの雨のカーテンの明るさ。0で描かない
    float curtainStart;      // カーテンのいちばん手前の層の距離 (m)
    float rainProjectionZScale;  // 射影行列の m22。距離から深度を出す
    float rainProjectionZOffset; // 射影行列の m32
    uint sprayWheelCount;    // 水はねを出すタイヤの数
    float sprayBrightness;   // 水はねの明るさの倍率
    float dripAmount;        // 雨だれの量 (0〜1)。垂れる場所の割合
    float3 rainPad;
};

// RainRenderer.h の DripSite と並びが一致していること
struct RainDripSite
{
    float4 site;  // xz = 垂れる所、y = 垂れ始める高さ、w = 落ちる先の高さ
    int2 cell;    // 探したマス
    float seed;   // 垂れる間隔などのばらつき
    uint state;   // 0 = まだ探していない、1 = 垂れない、2 = 垂れる
};

// RainRenderer.h の SprayWheel と並びが一致していること
struct RainSprayWheel
{
    float4 position; // xyz = 接地点、w = タイヤの幅
    float4 velocity; // xyz = 車の速さ、w = 未使用
};

struct RainVertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR0;   // 雨粒が受けている光 × 薄さ
    float across : TEXCOORD0; // 筋の太さ方向の位置 (-1〜1)
    float along : TEXCOORD1;  // 筋の長さ方向の位置 (0=先頭, 1=後ろ)
};

#ifdef RAIN_LIGHTS
#include "../Common/ClusteredLights.hlsli"

ConstantBuffer<ClusterParams> gClusterParams : register(b1);
StructuredBuffer<ClusteredLight> gClusteredLights : register(t0, space10);
StructuredBuffer<uint> gClusterCounts : register(t1, space10);
StructuredBuffer<uint> gClusterIndices : register(t2, space10);

// 1つの雨粒が拾うライトの上限
static const uint kRainMaxLights = 8;

float3 RainHash3(uint n)
{
    n = n * 747796405u + 2891336453u;
    uint3 v = uint3(n, n * 1664525u + 1013904223u, n * 22695477u + 1u);
    v = (v ^ (v >> 16u)) * 0x45d9f3bu;
    v = (v ^ (v >> 16u)) * 0x45d9f3bu;
    v ^= v >> 16u;
    return float3(v & 0xFFFFFFu) / float(0x1000000);
}

/// <summary>水滴が受ける光。光源の向こうから見ると明るい</summary>
/// <param name="pixel">水滴が写る画素 (クラスタを引く)</param>
/// <param name="maxLights">拾うライトの上限</param>
float3 RainLightAtPixel(float3 position, float3 toEye, float2 pixel, uint maxLights)
{
    float3 light = rainAmbient * 0.25f;
    if (gClusterParams.lightCount == 0 || any(pixel < 0.0f) || any(pixel >= rainScreenSize))
    {
        return light;
    }
    float viewZ = mul(float4(position, 1.0f), gClusterParams.view).z;
    uint cluster = ClusterIndex(pixel, viewZ, gClusterParams);
    uint count = min(gClusterCounts[cluster], maxLights);
    for (uint c = 0; c < count; c++)
    {
        ClusteredLight l = gClusteredLights[gClusterIndices[cluster * kMaxLightsPerCluster + c]];
        float3 L;
        float lightDistance;
        float attenuation = ClusteredLightShapeAttenuation(l, position, L, lightDistance);
        if (attenuation <= 0.0f)
        {
            continue;
        }
        float forward = saturate(dot(-L, toEye));
        light += l.color * attenuation * (0.3f + 2.0f * forward * forward * forward);
    }
    return light;
}

/// <param name="clip">水滴の位置を射影した座標</param>
float3 RainDropLight(float3 position, float3 toEye, float4 clip)
{
    if (clip.w <= 0.0f)
    {
        return rainAmbient * 0.25f;
    }
    float2 ndc = clip.xy / clip.w;
    float2 pixel = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f) * rainScreenSize;
    return RainLightAtPixel(position, toEye, pixel, kRainMaxLights);
}

/// <summary>明るさを頭打ちにする</summary>
float3 RainSoftClamp(float3 color)
{
    return color / (1.0f + max(max(color.r, color.g), color.b) * 2.0f);
}
#endif // RAIN_LIGHTS

#endif // RAIN_HLSLI
