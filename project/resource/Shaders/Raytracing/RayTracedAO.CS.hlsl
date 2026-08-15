#include "../Object/Object3d.hlsli"
#include "../Common/RayTracedAO.hlsli"
#include "../Common/GBufferUtil.hlsli"

// G-Bufferからワールド座標を復元し、画素ごとにAOを計算して1枚のテクスチャへ書き出す。
// TLASは t0,space2 固定。このCSには gTextures[] が無いのでG-Bufferは space0 を使ってよい

ConstantBuffer<AllLights> gLights : register(b0);

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b1)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

cbuffer AOPassParam : register(b2)
{
    uint2 screenSize;
    float temporalJitter; // フレームごとにサンプルの向きをずらす量。[0,1)
    uint aoPad;
};

Texture2D<float4> gGBufferNormal : register(t0);
Texture2D<float> gGBufferDepth : register(t1);

RWTexture2D<float> gAOOutput : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= screenSize.x || pixel.y >= screenSize.y)
    {
        return;
    }

    // 面が無い画素(空)は遮蔽しようがないので開けている扱い
    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gAOOutput[pixel] = 1.0f;
        return;
    }

    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);

    // 毎フレーム向きを変えて時間方向の蓄積で均す。
    // ずらし量をワールド座標側に足すと位置の精度を食うので、ハッシュの出力側で足す
    float seed = frac(rand3dTo1d(worldPos) + temporalJitter);

    gAOOutput[pixel] = TraceAO(worldPos, normal, gLights.aoSampleCount,
                               gLights.aoRadius, gLights.aoIntensity, seed);
}
