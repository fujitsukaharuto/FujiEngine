#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u0);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u1);
RWStructuredBuffer<Particle_Time> gParticles_Time : register(u2);
RWStructuredBuffer<Particle_Velocity> gParticles_Velocity : register(u3);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u4);
RWStructuredBuffer<Particle_Flags> gParticles_Flags : register(u5);

struct EmitterSurface
{
    float3 translate;
    float3 scale;
    float radius;
    uint count;
    float lifeTime;
    float frequency;
    float frequencyTime;
    uint emit;
    float3 colorMax;
    float3 colorMin;
    float3 baseVelocity;
    float velocityRandMax;
    float velocityRandMin;
    uint triangleCount;
};
struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
};
StructuredBuffer<VertexData> gVertices : register(t0);
StructuredBuffer<uint> gIndices : register(t1);
StructuredBuffer<float> gTriangleCDF : register(t2);

ConstantBuffer<EmitterSurface> gEmitter : register(b0);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

float3 RandomPointOnTriangle(inout RandomGenerator gen, float3 v0, float3 v1, float3 v2)
{
    float u = gen.Generate1d();
    float v = gen.Generate1d();
    if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
    return v0 + u * (v1 - v0) + v * (v2 - v0);
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    bool willEmit = (gEmitter.emit != 0) && (DTid.x < gEmitter.count);

    uint waveEmitCount = WaveActiveCountBits(willEmit);
    uint wavePrefix = WavePrefixCountBits(willEmit);
    int waveBaseHead = 0;

    if (WaveIsFirstLane() && waveEmitCount > 0)
    {
        InterlockedAdd(gFreeListIndex[0], (int)waveEmitCount, waveBaseHead);
        
        uint tailVal = (uint)gFreeListTailIndex[0];
        uint headVal = (uint)waveBaseHead;
        uint available = tailVal - headVal;

        if (waveEmitCount > available)
        {
            InterlockedAdd(gFreeListIndex[0], -(int)waveEmitCount);
            waveBaseHead = -1;
        }
    }

    waveBaseHead = WaveReadLaneFirst(waveBaseHead);

    if (waveBaseHead != -1 && willEmit)
    {
        uint headOld = (uint)waveBaseHead + wavePrefix;
        uint freePos = headOld % kMaxParticles;
        uint particleIndex = gFreeList[freePos];

        RandomGenerator generator;
        generator.InitSeed(DTid, gPerFrame.time);
        
        float r = generator.Generate1d();
        uint left = 0, right = gEmitter.triangleCount;
        while (left < right)
        {
            uint mid = (left + right) / 2;
            if (r < gTriangleCDF[mid]) right = mid;
            else left = mid + 1;
        }
        uint triIndex = min(left, gEmitter.triangleCount - 1);

        float3 p0 = gVertices[gIndices[triIndex * 3 + 0]].position.xyz;
        float3 p1 = gVertices[gIndices[triIndex * 3 + 1]].position.xyz;
        float3 p2 = gVertices[gIndices[triIndex * 3 + 2]].position.xyz;
        float3 pos = RandomPointOnTriangle(generator, p0, p1, p2) * gEmitter.radius + gEmitter.translate;

        gParticles_Scale[particleIndex].packedScale = PackHalf2(float2(gEmitter.scale.x, gEmitter.scale.x));
        gParticles_Trans[particleIndex].packedPos = PackPos21(pos);

        float3 t = (generator.Generate3d() + 1) * 0.5f;
        gParticles_Color[particleIndex].color = PackRGBA8(float4(lerp(gEmitter.colorMin, gEmitter.colorMax, t), 1.0f));

        float veloT = generator.Generate1d();
        float3 dirRand = generator.GenerateUnitSphereDirection();
        float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
        gParticles_Velocity[particleIndex].packedVelocity = PackHalf3(gEmitter.baseVelocity + dirRand * speed);
        gParticles_Time[particleIndex].lifeTime = gEmitter.lifeTime;
        gParticles_Time[particleIndex].currentTime = 0.0f;
        gParticles_Flags[particleIndex].flags = 0;
    }
}
