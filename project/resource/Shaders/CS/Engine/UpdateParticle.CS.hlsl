#include "../../CSParticle.hlsli"

static const uint kMakParticles = 1024;
RWStructuredBuffer<Particle> gParticle : register(u0);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b0);

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
    }
}