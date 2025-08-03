#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle> gParticle : register(u0);
struct EmitterSphere
{
    float3 translate;
    float radius;
    uint count;
    float frequency;
    float frequencyTime;
    uint emit;
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
        {
            return;
        }

            int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
        if (0 <= freeListIndex && freeListIndex < kMaxParticles)
        {
            float3 dir = RandomUnitVector(generator);
            float3 pos = gEmitter.translate + dir * gEmitter.radius;

            uint particleIndex = gFreeList[freeListIndex];
            gParticle[particleIndex].scale = float3(0.1f, 0.1f, 0.1f);
            gParticle[particleIndex].translate = pos;
            gParticle[particleIndex].color.rgb = (generator.Generate3d() + 1) * 0.5f;
            gParticle[particleIndex].color.a = 1.0f;
            float3 velocityOffset = (generator.Generate3d()) * 0.01f;
            gParticle[particleIndex].velocity = normalize(dir) * 0.01f + velocityOffset;
            gParticle[particleIndex].lifeTime = 60.0f;
            gParticle[particleIndex].currentTime = 0.0f;
        }
        else
        {
            InterlockedAdd(gFreeListIndex[0], 1);
        }
        
    }
}