#ifndef SCREEN_PROBE_COMMON_HLSLI
#define SCREEN_PROBE_COMMON_HLSLI

// 画面のプローブによる間接光 (Lumen の Screen Probe Gather) の CS 側の共通部分
// 16x16 画素ごとのプローブの明るさを L1 の球面調和関数で持ち、画素は近くの4つから引く
#include "GBufferUtil.hlsli"

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b1)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// ScreenProbeGIPass.h の ScreenProbeParam と並びが一致していること
cbuffer ScreenProbeParam : register(b2)
{
    float4x4 prevViewProj;
    float4x4 prevInvViewProj;
    uint2 screenSize;
    uint2 probeCount;
    uint tileSize;
    uint frameIndex;      // 置き場所とレイの向きをずらす種
    uint hasHistory;      // 0なら時間方向の蓄積を見ない
    float maxHistory;     // 蓄積の上限フレーム数
    float maxRayDistance;
    float pixelsPerUnit;  // 距離1で長さ1が画面上で何画素か (縦)
    float probePad1;
    float probePad2;
};

// space0〜6 は使用済みなので space7
Texture2D<float4> gGBufferNormal : register(t0, space7);
Texture2D<float> gGBufferDepth : register(t1, space7);
Texture2D<float4> gGBufferNormalPrev : register(t2, space7);
Texture2D<float> gGBufferDepthPrev : register(t3, space7);

// プローブ1つあたりのレイ。C++ 側の kRaysPerProbe_ と一致していること
static const uint kScreenProbeRays = 64;
static const uint kScreenProbeRaysSqrt = 8;

// 球面調和関数の係数 (L0, L1)。RGB それぞれ4係数で、横に3枚
static const float kSHBasisL0 = 0.282095f;
static const float kSHBasisL1 = 0.488603f;

float4 SHBasis(float3 direction)
{
    return float4(kSHBasisL0, kSHBasisL1 * direction.y, kSHBasisL1 * direction.z, kSHBasisL1 * direction.x);
}

/// <summary>係数から、向き normal の面に回り込む光。アンビエントと同じ尺度</summary>
/// <remarks>余弦ローブの畳み込み (L0 が π、L1 が 2π/3) を π で割ってある</remarks>
float SHIrradiance(float4 sh, float3 normal)
{
    float4 basis = SHBasis(normal);
    return max(0.0f, sh.x * basis.x + (2.0f / 3.0f) * dot(sh.yzw, basis.yzw));
}

/// <summary>整数の種から [0,1) を2つ (PCG)</summary>
float2 ScreenProbeHash2(uint2 cell, uint seed)
{
    uint v = cell.x * 1973u + cell.y * 9277u + seed * 26699u;
    v = v * 747796405u + 2891336453u;
    uint w = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
    w = (w >> 22u) ^ w;
    uint v2 = w * 747796405u + 2891336453u;
    uint w2 = ((v2 >> ((v2 >> 28u) + 4u)) ^ v2) * 277803737u;
    w2 = (w2 >> 22u) ^ w2;
    return float2(w, w2) * 2.3283064365386963e-10f;
}

#endif // SCREEN_PROBE_COMMON_HLSLI
