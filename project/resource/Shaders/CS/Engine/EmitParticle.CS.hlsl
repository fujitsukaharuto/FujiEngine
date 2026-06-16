#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u0);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u1);
RWStructuredBuffer<Particle_Time> gParticles_Time : register(u2);
RWStructuredBuffer<Particle_Velocity> gParticles_Velocity : register(u3);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u4);
RWStructuredBuffer<Particle_Flags> gParticles_Flags : register(u5);

struct EmitterSphere
{
    float3 translate;
    float radius;
    float3 scale;
    uint count;
    float3 colorMax;
    float lifeTime;
    float3 colorMin;
    float frequency;
    float3 baseVelocity;
    float velocityRandMax;
    float velocityRandMin;
    uint emit;
    uint isDistance;
    float frequencyTime;
    float3 prevTranslate;
    uint emitShapeType;
    float4 rotation;
    uint emitVeloType;
    uint isRandomMove;
    uint isTrailEmit;
    uint isGravity;
};
ConstantBuffer<EmitterSphere> gEmitter : register(b0);

struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b1);

RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

float3 RandomUnitVector(inout RandomGenerator gen)
{
    float3 v = gen.Generate3d();
    if (length(v) < 0.0001f) v = gen.Generate3d();
    if (length(v) > 1.0f) v = gen.Generate3d();
    return normalize(v);
}

float3 GenerateEmitPosition(uint type, inout RandomGenerator generator)
{
    switch (type)
    {
        case 0: return RandomUnitVector(generator) * gEmitter.radius;
        case 1: 
        {
            float angle = generator.Generate1d() * 6.2831853f;
            return float3(cos(angle), 0, sin(angle)) * gEmitter.radius;
        }
        case 2: 
        {
            float3 p = float3(generator.Generate1d() * 2.0f - 1.0f, 0, generator.Generate1d() * 2.0f - 1.0f);
            return p * gEmitter.radius;
        }
        case 3: return generator.Generate3d() * gEmitter.radius;
        case 4: 
        {
            float3 dir = RandomUnitVector(generator);
            dir.y = abs(dir.y);
            return normalize(dir) * gEmitter.radius;
        }
        case 5: 
        {
            float angle = generator.Generate1d() * 6.2831853f;
            float r = gEmitter.radius;
            return float3(cos(angle) * r, generator.Generate1d() - 0.5f, sin(angle) * r);
        }
        case 6: 
        {
            float major = gEmitter.radius;
            float minor = 0.5f;
            float u = generator.Generate1d() * 6.2831853f;
            float v = generator.Generate1d() * 6.2831853f;
            return float3((major + minor * cos(v)) * cos(u), minor * sin(v), (major + minor * cos(v)) * sin(u));
        }
    }
    return RandomUnitVector(generator) * gEmitter.radius;
}

float3 GenerateEmitVelocity(uint type, float3 localPos, inout RandomGenerator generator)
{
    switch (type)
    {
        case 0: return RandomUnitVector(generator);
        case 1: return normalize(localPos);
        case 2: return -normalize(localPos);
        case 3: 
        {
            float3 p = normalize(float3(localPos.x, 0, localPos.z));
            return float3(-p.z, 0, p.x);
        }
        case 4: return normalize(float3(0, 1, 0) + RandomUnitVector(generator) * 0.2f);
    }
    return RandomUnitVector(generator);
}

float3 RotateVector(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    bool willEmit = (gEmitter.emit != 0) && (DTid.x < gEmitter.count);

    // Wave単位での集計
    uint waveEmitCount = WaveActiveCountBits(willEmit);
    uint wavePrefix = WavePrefixCountBits(willEmit);
    int waveBaseHead = 0;

    if (WaveIsFirstLane() && waveEmitCount > 0)
    {
        InterlockedAdd(gFreeListIndex[0], (int)waveEmitCount, waveBaseHead);
        
        // リングバッファ空き容量チェック (32bitラップアラウンド対応)
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
        
        float lerpT = (gEmitter.count > 1) ? (float)DTid.x / (gEmitter.count - 1) : 0.0f;
        float3 interpPos = (gEmitter.isDistance == 1) ? lerp(gEmitter.prevTranslate, gEmitter.translate, lerpT) : gEmitter.translate;

        float3 localPos = GenerateEmitPosition(gEmitter.emitShapeType, generator);
        localPos = RotateVector(localPos, gEmitter.rotation);
        float3 pos = interpPos + localPos;

        gParticles_Scale[particleIndex].packedScale = PackHalf2(float2(gEmitter.scale.x, gEmitter.scale.x));
        gParticles_Trans[particleIndex].translate = pos;
        gParticles_Trans[particleIndex].prevTranslate = pos;

        float3 t = (generator.Generate3d() + 1) * 0.5f;
        float4 color = float4(lerp(gEmitter.colorMin, gEmitter.colorMax, t), 1.0f);
        gParticles_Color[particleIndex].color = PackRGBA8(color);

        float veloT = generator.Generate1d();
        float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
        float3 velDir = GenerateEmitVelocity(gEmitter.emitVeloType, localPos, generator);
        gParticles_Velocity[particleIndex].packedVelocity = PackHalf3(gEmitter.baseVelocity + velDir * speed);

        gParticles_Time[particleIndex].lifeTime = gEmitter.lifeTime;
        gParticles_Time[particleIndex].currentTime = 0.0f;
        
        uint flags = (gEmitter.isRandomMove & 0x3);
        flags |= (gEmitter.isTrailEmit & 0x1) << 2;
        flags |= (gEmitter.isGravity & 0x1) << 3;
        gParticles_Flags[particleIndex].flags = flags;
    }
}
