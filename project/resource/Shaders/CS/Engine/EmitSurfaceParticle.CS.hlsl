#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle> gParticle : register(u0);
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
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);


float3 RandomUnitVector(RandomGenerator gen)
{
    float3 v;
    v = gen.Generate3d(); // [-1, 1]
    if (length(v) < 0.0001f)
    {
        v = gen.Generate3d();
    }
    if (length(v) > 1.0f)
    {
        v = gen.Generate3d();
    }
    return normalize(v);
}

float3 RandomPointOnTriangle(RandomGenerator gen, float3 v0, float3 v1, float3 v2)
{
    float u = gen.Generate1d();
    float v = gen.Generate1d();

    // u+v > 1 の場合、対角反転して内部に収める
    if (u + v > 1.0f)
    {
        u = 1.0f - u;
        v = 1.0f - v;
    }

    float3 p = v0 + u * (v1 - v0) + v * (v2 - v0);
    return p;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit != 0)
    {
        RandomGenerator generator;
        generator.InitSeed(DTid, gPerFrame.time);

        if (DTid.x >= gEmitter.count)
            return;

        int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
        if (0 <= freeListIndex && freeListIndex < kMaxParticles)
        {
            float r = generator.Generate1d();
            uint triIndex = 0;
            
            uint left = 0;
            uint right = gEmitter.triangleCount - 1;
            while (left < right)
            {
                uint mid = (left + right) / 2;
                if (r <= gTriangleCDF[mid])
                {
                    right = mid;
                }
                else
                {
                    left = mid + 1;
                }
            }
            triIndex = left;


            uint i0 = gIndices[triIndex * 3 + 0];
            uint i1 = gIndices[triIndex * 3 + 1];
            uint i2 = gIndices[triIndex * 3 + 2];
            float3 p0 = gVertices[i0].position.xyz;
            float3 p1 = gVertices[i1].position.xyz;
            float3 p2 = gVertices[i2].position.xyz;
            // 三角形内のランダム点
            float3 pos = RandomPointOnTriangle(generator, p0, p1, p2);
            // エミッタ座標系
            pos = pos * gEmitter.radius + gEmitter.translate;


            uint particleIndex = gFreeList[freeListIndex];
            gParticle[particleIndex].scale = gEmitter.scale;
            gParticle[particleIndex].startScale = gEmitter.scale;
            gParticle[particleIndex].translate = pos;

            float3 t = (generator.Generate3d() + 1) * 0.5f;
            gParticle[particleIndex].color.rgb = lerp(gEmitter.colorMin, gEmitter.colorMax, t);
            gParticle[particleIndex].color.a = 1.0f;

            float veloT = generator.Generate1d();
            float3 dirRand = generator.GenerateUnitSphereDirection();
            float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
            float3 velocityOffset = gEmitter.baseVelocity + dirRand * speed;
            gParticle[particleIndex].velocity = velocityOffset;

            gParticle[particleIndex].lifeTime = gEmitter.lifeTime;
            gParticle[particleIndex].currentTime = 0.0f;
        }
        else
        {
            InterlockedAdd(gFreeListIndex[0], 1);
        }
    }
}