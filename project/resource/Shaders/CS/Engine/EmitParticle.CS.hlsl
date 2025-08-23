#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle> gParticle : register(u0);
struct EmitterSphere
{
    float3 translate;
    float3 scale;
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


[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit != 0)
    {
        RandomGenerator generator;
        generator.InitSeed(DTid, gPerFrame.time);

        if (DTid.x >= gEmitter.count)
            return;

        int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
        if (0 <= freeListIndex && freeListIndex < kMaxParticles)
        {
            float3 dir = RandomUnitVector(generator);

            // 補間係数（0〜1）: スレッドIDをcountで割る
            float lerpT;
            if (gEmitter.count > 1)
            {
                lerpT = (float) DTid.x / (gEmitter.count - 1);
            }
            else
            {
                lerpT = 0.0f;
            }
            
            // 位置の補間
            float3 interpPos = lerp(gEmitter.prevTranslate, gEmitter.translate, lerpT);
            float3 pos = interpPos + dir * gEmitter.radius;

            uint particleIndex = gFreeList[freeListIndex];
            gParticle[particleIndex].scale = gEmitter.scale;
            gParticle[particleIndex].startScale = gEmitter.scale;
            gParticle[particleIndex].translate = pos;

            float3 t = (generator.Generate3d() + 1) * 0.5f;
            gParticle[particleIndex].color.rgb = lerp(gEmitter.colorMin, gEmitter.colorMax, t);
            gParticle[particleIndex].color.a = 1.0f;

            float veloT = generator.Generate1d();
            float3 dirRand = generator.GenerateUnitSphereDirection();
            float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
            float3 velocityOffset = gEmitter.baseVelocity + dirRand * speed;
            gParticle[particleIndex].velocity = velocityOffset;

            gParticle[particleIndex].lifeTime = gEmitter.lifeTime;
            gParticle[particleIndex].currentTime = 0.0f;
        }
        else
        {
            InterlockedAdd(gFreeListIndex[0], 1);
        }
    }
}