#include "../../CSParticle.hlsli"
#include "../Noise.hlsli"

RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u0);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u1);
RWStructuredBuffer<Particle_Time> gParticles_Time : register(u2);
RWStructuredBuffer<Particle_Velocity> gParticles_Velocity : register(u3);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u4);
RWStructuredBuffer<Particle_Flags> gParticles_Flags : register(u5);
RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

void EmitTrail(uint pIndex)
{
    float dist = length(gParticles_Trans[pIndex].translate - gParticles_Trans[pIndex].prevTranslate);
    if (dist > 0.01f)
    {
        // トレイル粒子生成回数 (距離に応じて 1~n 個)
        int numTrail = (int) (dist * 100.0f * 1.0f);
        numTrail = clamp(numTrail, 0, 60);
        if (numTrail == 0)
            return;

        uint originalHead;
        InterlockedAdd(gFreeListIndex[0], numTrail, originalHead);
        uint newHead = originalHead + (numTrail - 1);
        uint capacity = kMaxParticles;
        uint tail = gFreeListTailIndex[0];
        if (newHead >= tail)
        {
            uint dummy;
            InterlockedAdd(gFreeListIndex[0], -numTrail, dummy);
            return;
        }

        for (int t = 0; t < numTrail; ++t)
        {
            int slot = (originalHead + t) % capacity;
            uint trailIndex = gFreeList[slot];

            float k = (float) t / max(1, numTrail);
            float3 trailPos = lerp(gParticles_Trans[pIndex].prevTranslate, gParticles_Trans[pIndex].translate, k);

            gParticles_Trans[trailIndex].translate = trailPos;
            gParticles_Trans[trailIndex].prevTranslate = trailPos;
            gParticles_Scale[trailIndex].scale = gParticles_Scale[pIndex].scale * 0.75f;
            gParticles_Scale[trailIndex].startScale = gParticles_Scale[pIndex].scale * 0.75f;
            gParticles_Time[trailIndex].lifeTime = gParticles_Time[pIndex].lifeTime * 0.5f;
            gParticles_Time[trailIndex].currentTime = 0;
            gParticles_Velocity[trailIndex].velocity = float3(0, 0, 0);
            gParticles_Color[trailIndex].color = gParticles_Color[pIndex].color;
            gParticles_Color[trailIndex].color.a = 1.0f;
            gParticles_Flags[trailIndex].isRandomMove = 0;
            gParticles_Flags[trailIndex].isTrailEmit = 0;
            gParticles_Flags[trailIndex].isGravity = 0;
        }
    }
}

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        if (gParticles_Color[particleIndex].color.a != 0)
        {
            if (gParticles_Flags[particleIndex].isTrailEmit == 1)
            {
                EmitTrail(particleIndex);
            }
        }
    }
}