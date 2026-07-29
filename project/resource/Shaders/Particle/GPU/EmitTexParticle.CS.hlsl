#include "CSParticle.hlsli"
#include "../../Common/Random.hlsli"

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
    float3 prevTranslate;
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

Texture2D<float4> gMaskTex : register(t0);
SamplerState gSampler : register(s0);

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit == 0) return;

    int width, height;
    gMaskTex.GetDimensions(width, height);
    if (DTid.x >= (uint)width || DTid.y >= (uint)height) return;

    float2 uv = (DTid.xy + 0.5) / float2(width, height);
    float4 maskColor = gMaskTex.SampleLevel(gSampler, uv, 0);
    float luminance = dot(maskColor.rgb, float3(0.299, 0.587, 0.114));

    bool willEmit = (luminance >= 0.6f);

    // Wave集計
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

        float3 worldPos = gEmitter.translate + float3((uv.x - 0.5f) * gEmitter.radius * 2.0f, 0, (uv.y - 0.5f) * gEmitter.radius * 2.0f);
        gParticles_Trans[particleIndex].packedPos = PackPos21(worldPos);
        gParticles_Scale[particleIndex].packedScale = PackHalf2(float2(0.1f, 0.1f));

        RandomGenerator generator;
        generator.InitSeed(DTid, gPerFrame.time);
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
