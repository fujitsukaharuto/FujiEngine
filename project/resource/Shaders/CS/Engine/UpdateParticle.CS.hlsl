#include "../../CSParticle.hlsli"

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
    if (particleIndex < kMaxParticles)
    {
        if (gParticle[particleIndex].color.a != 0)
        {
            gParticle[particleIndex].translate += gParticle[particleIndex].velocity;
            gParticle[particleIndex].currentTime += gPerFrame.deltaTime;
            float lifeRatio = gParticle[particleIndex].currentTime / gParticle[particleIndex].lifeTime;

            float alpha = 1.0f - lifeRatio;
            gParticle[particleIndex].color.a = saturate(alpha);

            gParticle[particleIndex].scale = gParticle[particleIndex].startScale * (1.0f - lifeRatio);
        }
        if (gParticle[particleIndex].color.a == 0.0f)
        {
            gParticle[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
            
            if ((freeListIndex + 1) < kMaxParticles)
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