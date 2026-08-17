#include "../Object/Object3d.hlsli"
#include "../Common/RayTracedShadow.hlsli"
#include "../Common/GBufferUtil.hlsli"
#include "../Common/Random.hlsli"

// G-Bufferからワールド座標を復元し、画素ごとに平行光源(0番)の遮蔽率を1枚のテクスチャへ書き出す。
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

cbuffer ShadowPassParam : register(b2)
{
    uint2 screenSize;
    float temporalJitter; // フレームごとにサンプルの向きをずらす量。[0,1)
    uint shadowPad;
};

Texture2D<float4> gGBufferNormal : register(t0);
Texture2D<float> gGBufferDepth : register(t1);

RWTexture2D<float> gShadowOutput : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= screenSize.x || pixel.y >= screenSize.y)
    {
        return;
    }

    // 面が無い画素(空)は影の落ちようがないので当たっている扱い
    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gShadowOutput[pixel] = 1.0f;
        return;
    }

    if (gLights.numDirectionalLights <= 0 || (gLights.rayTracedShadowMask & kShadowMaskDirectional) == 0)
    {
        gShadowOutput[pixel] = 1.0f;
        return;
    }

    float3 L = normalize(-gLights.directionalLights[0].direction);
    // 光源が地平線の下に丸ごと沈んでいる面は、どのみち全てのレイが自分の裏側へ向く。
    // 境目ちょうどでは光源の一部だけが見えている＝半影なので、円錐の広がりぶんは残すこと
    if (dot(normal, L) <= -sin(gLights.sunAngularRadius))
    {
        gShadowOutput[pixel] = 0.0f;
        return;
    }

    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);

    // 毎フレーム向きを変えて時間方向の蓄積で均す。
    // ずらし量をワールド座標側に足すと位置の精度を食うので、ハッシュの出力側で足す
    float seed = frac(rand3dTo1d(worldPos) + temporalJitter);

    gShadowOutput[pixel] = TraceSoftShadowDirectional(worldPos, L, normal,
                                                      gLights.sunAngularRadius,
                                                      gLights.shadowSampleCount, seed);
}
