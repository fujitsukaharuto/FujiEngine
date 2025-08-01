#include "../../CSParticle.hlsli"

static const uint kMakParticles = 1024;
RWStructuredBuffer<Particle> gParticle : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex == 0)
    {
        gFreeListIndex[0] = kMakParticles - 1;
    }
    if (particleIndex < kMakParticles)
    {
        gParticle[particleIndex] = (Particle) 0;
        gFreeList[particleIndex] = particleIndex;
    }
}