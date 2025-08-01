#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

static const uint kMakParticles = 1024;
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


[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit != 0)
    {
        RandomGenerator generator;
        generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;
        for (uint countIndex = 0; countIndex < gEmitter.count; ++countIndex)
        {
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            if (0 <= freeListIndex && freeListIndex < kMakParticles)
            {
                uint particleIndex = gFreeList[freeListIndex];
                gParticle[particleIndex].scale = generator.Generate3d();
                gParticle[particleIndex].translate = generator.Generate3d();
                gParticle[particleIndex].color.rgb = generator.Generate3d();
                gParticle[particleIndex].color.a = 1.0f;
                gParticle[particleIndex].velocity = (generator.Generate3d() * 0.1f) - 0.05f;
                gParticle[particleIndex].lifeTime = 60.0f;
                gParticle[particleIndex].currentTime = 0.0f;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], 1);
                break;
            }
        }
    }
}