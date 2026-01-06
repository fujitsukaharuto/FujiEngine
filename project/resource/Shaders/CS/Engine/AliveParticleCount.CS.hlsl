#include "../../CSParticle.hlsli"

RWStructuredBuffer<Particle_Color> gParticles_Color : register(u0);
RWStructuredBuffer<DrawIndexedArgs> gDrawArgs : register(u1);
RWStructuredBuffer<uint> gDrawParticleIndex : register(u2);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex >= kMaxParticles)
        return;
    if (gParticles_Color[particleIndex].color.a >= 0.15)
    {
        uint writeIndex;
        InterlockedAdd(gDrawArgs[0].InstanceCount, 1, writeIndex);
        gDrawParticleIndex[writeIndex] = particleIndex;
    }
}