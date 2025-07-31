#include "../../CSParticle.hlsli"

static const uint kMakParticles = 1024;
RWStructuredBuffer<Particle> gParticle : register(u0);
RWStructuredBuffer<int> gFreeCount : register(u1);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex == 0)
    {
        gFreeCount[0] = 0;
    }
    if (particleIndex < kMakParticles)
    {
        gParticle[particleIndex] = (Particle) 0;
    }
}