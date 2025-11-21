#include "../../CSParticle.hlsli"
#include "../Noise.hlsli"

RWStructuredBuffer<Particle> gParticle : register(u0);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

void MoveMode(uint pIndex)
{
    if (gParticle[pIndex].isRandomMove == 1)
    {
        float3 pos = gParticle[pIndex].translate;
        float time = gPerFrame.time;

        float3 samplePos = pos * 0.4 + float3(0, time * pIndex, 0);
        float3 curl = CurlNoise(samplePos);

        float3 vel0 = gParticle[pIndex].velocity;
        if (length(vel0) < 0.0001f)
        {
            float3 seed = pos * 0.3 + float3(1.234, 5.678, 9.1011);
            float3 rnd = CurlNoise(seed);
            vel0 = normalize(rnd + float3(0.1, 0.2, 0.1)) * 0.005f;
        }

        float baseSpeed = length(vel0);
        if (baseSpeed < 0.0001f)
            baseSpeed = 0.01f;

        float3 baseDir = normalize(vel0);
        float3 force = curl * 0.2;
        float3 vel = baseDir + force;

        vel = normalize(vel);
        gParticle[pIndex].velocity = vel * baseSpeed;
    }
    else if (gParticle[pIndex].isRandomMove == 2)
    {
        float3 pos = gParticle[pIndex].translate;
        float3 curl = CurlNoise(pos * 0.5);

        float noisePower = 0.2f; // ノイズ強度
        float speed = length(gParticle[pIndex].velocity);

        gParticle[pIndex].velocity =
        normalize(gParticle[pIndex].velocity + curl * noisePower) * speed;
    }

}

void EmitTrail(uint pIndex)
{
    float dist = length(gParticle[pIndex].translate - gParticle[pIndex].prevTranslate);
    if (dist > 0.01f)
    {
        // トレイル粒子生成回数 (距離に応じて 1~n 個)
        int numTrail = (int) (dist * 100.0f * 1.0f);
        numTrail = clamp(numTrail, 0, 10);
        for (int t = 0; t < numTrail; t++)
        {
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            if (freeListIndex >= 0 && freeListIndex < kMaxParticles)
            {
                uint trailIndex = gFreeList[freeListIndex];

                float k = (float) t / max(1, numTrail);
                float3 trailPos = lerp(gParticle[pIndex].prevTranslate, gParticle[pIndex].translate, k);

                gParticle[trailIndex].translate = trailPos;
                gParticle[trailIndex].scale = gParticle[pIndex].scale * 0.75f;
                gParticle[trailIndex].startScale = gParticle[pIndex].scale * 0.75f;

                gParticle[trailIndex].velocity = float3(0, 0, 0);
                gParticle[trailIndex].color = gParticle[pIndex].color;
                gParticle[trailIndex].color.a = 1.0f;
                gParticle[trailIndex].lifeTime = gParticle[pIndex].lifeTime * 0.5f;
                gParticle[trailIndex].currentTime = 0;
                gParticle[trailIndex].isRandomMove = 0;
                gParticle[trailIndex].isTrailEmit = 0;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], 1);
            }
        }
    }
}

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        if (gParticle[particleIndex].color.a != 0)
        {
            if (gParticle[particleIndex].isRandomMove != 0)
            {
                MoveMode(particleIndex);
            }

            gParticle[particleIndex].prevTranslate = gParticle[particleIndex].translate;
            gParticle[particleIndex].translate += gParticle[particleIndex].velocity;
            
            if (gParticle[particleIndex].isTrailEmit == 1)
            {
                EmitTrail(particleIndex);
            }
            
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