#include "../../CSParticle.hlsli"


RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u0);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u1);
RWStructuredBuffer<Particle_Time> gParticles_Time : register(u2);
RWStructuredBuffer<Particle_Velocity> gParticles_Velocity : register(u3);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u4);
RWStructuredBuffer<Particle_Flags> gParticles_Flags : register(u5);
RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        gParticles_Trans[particleIndex] = (Particle_Translate) 0;
        gParticles_Scale[particleIndex] = (Particle_Scale) 0;
        gParticles_Time[particleIndex] = (Particle_Time) 0;
        gParticles_Velocity[particleIndex] = (Particle_Velocity) 0;
        gParticles_Color[particleIndex] = (Particle_Color) 0;
        gParticles_Flags[particleIndex] = (Particle_Flags) 0;
        gFreeList[particleIndex] = particleIndex;
        if (particleIndex == 0)
        {
            gFreeListIndex[0] = 0;
            gFreeListTailIndex[0] = kMaxParticles;
        }
    }
}