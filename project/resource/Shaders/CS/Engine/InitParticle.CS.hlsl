#include "../../CSParticle.hlsli"


RWStructuredBuffer<Particle> gParticle : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    gFreeListIndex[0] = 0;
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        gParticle[particleIndex] = (Particle) 0;
        gFreeList[particleIndex] = particleIndex;
        if (particleIndex == 0)
        {
            gFreeListIndex[0] = kMaxParticles - 1;
        }
    }
}