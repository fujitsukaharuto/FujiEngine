#include "../Common/GBufferUtil.hlsli"

// デノイザの空間方向。à-trous を1回ぶん、横か縦の片方向だけ掛ける。
// 5x5 の B-スプラインは1次元カーネルの外積なので 25タップが 5+5 タップになる

// Camera.h の CameraInfo と並びが一致していること
cbuffer CameraInfo : register(b0)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPad;
    float4x4 viewProj;
};

// Denoiser.h の DenoiseSpatialParam と並びが一致していること
cbuffer DenoiseSpatialParam : register(b1)
{
    uint2 screenSize;
    uint stepWidth;    // タップの間隔。通すたびに倍にする
    float normalPower; // 法線の一致度に掛ける指数。大きいほど向きの違いに厳しい
    float planeScale;  // 面からの許容距離。カメラからの距離と stepWidth に対する割合
    int2 tapDirection; // タップを並べる向き。(1,0)で横、(0,1)で縦
    float spatialPad;
};

Texture2D<float4> gGBufferNormal : register(t0);
Texture2D<float> gGBufferDepth : register(t1);
Texture2D<float> gDenoiseFilterInput : register(t2);

RWTexture2D<float> gDenoiseFilterOut : register(u0);

// B-スプラインの5タップ。これを横と縦に1回ずつ通すと 5x5 の基本カーネルになる
static const float kKernel[5] = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= screenSize.x || pixel.y >= screenSize.y)
    {
        return;
    }

    float center = gDenoiseFilterInput.Load(int3(pixel, 0)).r;

    float3 normal;
    float depth;
    if (!LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gDenoiseFilterOut[pixel] = center;
        return;
    }

    float3 worldPos = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);

    // 間隔を広げるとタップが幾何的にも遠くなる。固定にすると斜面で後半の通過が丸ごと弾かれる
    float planeTolerance = planeScale * length(worldPos - cameraPos) * float(stepWidth);

    float sum = 0.0f;
    float weightSum = 0.0f;

    for (int i = -2; i <= 2; i++)
    {
        int2 tap = int2(pixel) + tapDirection * (i * int(stepWidth));
        if (tap.x < 0 || tap.y < 0 || tap.x >= int(screenSize.x) || tap.y >= int(screenSize.y))
        {
            continue;
        }

        float3 tapNormal;
        float tapDepth;
        if (!LoadSurface(gGBufferNormal, gGBufferDepth, tap, tapNormal, tapDepth))
        {
            continue;
        }

        float normalWeight = pow(saturate(dot(normal, tapNormal)), normalPower);

        // 中心の面から法線方向にどれだけ浮いているか。同一平面なら遠くても0で、段差で跳ね上がる
        float3 tapWorldPos = ReconstructWorldPos(uint2(tap), screenSize, tapDepth, invViewProj);
        float planeDistance = abs(dot(normal, tapWorldPos - worldPos));
        float planeWeight = exp(-planeDistance / max(planeTolerance, 1e-4f));

        float weight = kKernel[i + 2] * normalWeight * planeWeight;

        sum += weight * gDenoiseFilterInput.Load(int3(tap, 0)).r;
        weightSum += weight;
    }

    gDenoiseFilterOut[pixel] = (weightSum > 0.0f) ? (sum / weightSum) : center;
}
