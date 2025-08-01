#include "../../CSParticle.hlsli"

static const uint kMakParticles = 1024;
RWStructuredBuffer<Particle> gParticle : register(u0);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMakParticles)
    {
        if (gParticle[particleIndex].color.a != 0)
        {
            gParticle[particleIndex].translate += gParticle[particleIndex].velocity;
            gParticle[particleIndex].currentTime += gPerFrame.deltaTime;
            float alpha = 1.0f - (gParticle[particleIndex].currentTime / gParticle[particleIndex].lifeTime);
            gParticle[particleIndex].color.a = saturate(alpha);
        }
        if (gParticle[particleIndex].color.a == 0)
        {
            gParticle[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
            
            if ((freeListIndex + 1) < kMakParticles)
            {
                gFreeList[freeListIndex + 1] = particleIndex;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            }
        }
    }
}