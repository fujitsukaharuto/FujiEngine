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


[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{

    if (gEmitter.emit == 0)
        return;

    int width, height;
    gMaskTex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    // UV算出
    float2 uv = (DTid.xy + 0.5) / float2(width, height);
    float4 maskColor = gMaskTex.SampleLevel(gSampler, uv, 0);

    // Emit条件: 輝度が一定以上
    float luminance = dot(maskColor.rgb, float3(0.299, 0.587, 0.114));
    if (luminance < 0.6f)
        return; // ここでフィルタリング

    uint headOld;
    InterlockedAdd(gFreeListIndex[0], 1, headOld);
    uint freePos = headOld % kMaxParticles;
    uint particleIndex = gFreeList[freePos];
        // もし freeList が枯渇している場合のチェック
        // headOld と tailの差で検出できる
    uint tailVal = gFreeListTailIndex[0];
    if (headOld >= tailVal)
    {
            // 空き無し → 元に戻す
        InterlockedAdd(gFreeListIndex[0], -1);
        return;
    }

    // テクスチャ座標をワールド位置にマッピング
    float3 worldPos = gEmitter.translate + 
    float3((uv.x - 0.5f) * gEmitter.radius * 2.0f,
    0,
    (uv.y - 0.5f) * gEmitter.radius * 2.0f);

    gParticles_Trans[particleIndex].translate = worldPos;

    gParticles_Scale[particleIndex].scale = float3(0.1f, 0.1f, 0.1f);
    gParticles_Scale[particleIndex].startScale = gParticles_Scale[particleIndex].scale;

    // 色はマスク色 or ランダム
    RandomGenerator generator;
    generator.InitSeed(DTid, gPerFrame.time);
    float3 t = (generator.Generate3d() + 1) * 0.5f;
    gParticles_Color[particleIndex].color.rgb = lerp(gEmitter.colorMin, gEmitter.colorMax, t);
    gParticles_Color[particleIndex].color.a = 1.0f;

    float veloT = generator.Generate1d();
    float3 dirRand = generator.GenerateUnitSphereDirection();
    float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
    gParticles_Velocity[particleIndex].velocity = gEmitter.baseVelocity + dirRand * speed;

    gParticles_Time[particleIndex].lifeTime = gEmitter.lifeTime;
    gParticles_Time[particleIndex].currentTime = 0.0f;
    
    gParticles_Flags[particleIndex].isRandomMove = 0;
    gParticles_Flags[particleIndex].isTrailEmit = 0;
    gParticles_Flags[particleIndex].isGravity = 0;
}