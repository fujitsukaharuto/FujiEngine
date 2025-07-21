struct Particle
{
    float3 translate;
    float3 scale;
    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;
};
static const uint kMakParticles = 1024;
RWStructuredBuffer<Particle> gParticle : register(u0);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMakParticles)
    {
        gParticle[particleIndex] = (Particle) 0;
    }
}