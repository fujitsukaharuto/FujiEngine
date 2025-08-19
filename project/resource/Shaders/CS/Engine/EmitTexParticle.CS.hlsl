#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle> gParticle : register(u0);
struct EmitterSphere
{
    float3 translate;
    float radius;
    uint count;
    float lifeTime;
    float frequency;
    float frequencyTime;
    uint emit;
    
    float3 colorMax;
    float3 colorMin;
    
    float3 baseVelocity;
    float velocityRandMax;
    float velocityRandMin;

    float3 prevTranslate;

};
ConstantBuffer<EmitterSphere> gEmitter : register(b0);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

Texture2D<float4> gMaskTex : register(t0);
SamplerState gSampler : register(s0);

float3 RandomUnitVector(RandomGenerator gen)
{
    float3 v;
    v = gen.Generate3d(); // [-1, 1]
    if (length(v) < 0.0001f)
    {
        v = gen.Generate3d();
    }
    if (length(v) > 1.0f)
    {
        v = gen.Generate3d();
    }
    return normalize(v);
}


[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{

    if (gEmitter.emit == 0)
        return;

    int width, height;
    gMaskTex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    // UV算出
    float2 uv = (DTid.xy + 0.5) / float2(width, height);
    float4 maskColor = gMaskTex.SampleLevel(gSampler, uv, 0);

    // Emit条件: 輝度が一定以上
    float luminance = dot(maskColor.rgb, float3(0.299, 0.587, 0.114));
    if (luminance < 0.6f)
        return; // ここでフィルタリング

    // FreeListからインデックス確保
    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
    if (0 <= freeListIndex && freeListIndex < kMaxParticles)
    {
        uint particleIndex = gFreeList[freeListIndex];

        // テクスチャ座標をワールド位置にマッピング
        float3 worldPos = gEmitter.translate 
                        + float3((uv.x - 0.5f) * gEmitter.radius * 2.0f,
                                 0,
                                 (uv.y - 0.5f) * gEmitter.radius * 2.0f);

        gParticle[particleIndex].translate = worldPos;

        gParticle[particleIndex].scale = float3(0.1f, 0.1f, 0.1f);
        gParticle[particleIndex].startScale = gParticle[particleIndex].scale;

        // 色はマスク色 or ランダム
        RandomGenerator generator;
        generator.InitSeed(DTid, gPerFrame.time);
        float3 t = (generator.Generate3d() + 1) * 0.5f;
        gParticle[particleIndex].color.rgb = lerp(gEmitter.colorMin, gEmitter.colorMax, t);
        gParticle[particleIndex].color.a = 1.0f;


        float veloT = generator.Generate1d();
        float3 dirRand = generator.GenerateUnitSphereDirection();
        float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
        gParticle[particleIndex].velocity = gEmitter.baseVelocity + dirRand * speed;

        gParticle[particleIndex].lifeTime = gEmitter.lifeTime;
        gParticle[particleIndex].currentTime = 0.0f;
    }
    else
    {
        InterlockedAdd(gFreeListIndex[0], 1);
    }
}