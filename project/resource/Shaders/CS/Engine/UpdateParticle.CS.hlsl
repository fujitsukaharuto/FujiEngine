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
RWStructuredBuffer<int> gFreeListTailIndex : register(u3);

void MoveMode(uint pIndex)
{
    if (gParticle[pIndex].isRandomMove == 1)
    {
        float3 pos = gParticle[pIndex].translate;
        float time = gPerFrame.time;

        float3 samplePos = pos * 0.4 + float3(0, time * pIndex, 0);
        float3 curl = CurlNoise(samplePos);

        float3 vel0 = gParticle[pIndex].velocity;
        float len0 = length(vel0);
        if (len0 < 0.0001f)
        {
            float3 seed = pos * 0.3 + float3(1.234, 5.678, 9.1011);
            float3 rnd = CurlNoise(seed);
            float3 v = rnd + float3(0.1, 0.2, 0.1);
            float l = length(v);
            if (l < 0.0001f)
                v = float3(0.3, 0.1, 0.2);
            vel0 = v / length(v) * 0.005f;
            len0 = 0.005f;
        }

        float baseSpeed = len0;
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

        float noisePower = 4.0f; // ノイズ強度
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
            float3 trailPos = lerp(gParticle[pIndex].prevTranslate,gParticle[pIndex].translate,k);
            Particle src = gParticle[pIndex];
            gParticle[trailIndex].translate = trailPos;
            gParticle[trailIndex].scale = src.scale * 0.75f;
            gParticle[trailIndex].startScale = src.scale * 0.75f;
            gParticle[trailIndex].velocity = float3(0, 0, 0);
            gParticle[trailIndex].color = src.color;
            gParticle[trailIndex].color.a = 1.0f;
            gParticle[trailIndex].lifeTime = src.lifeTime * 0.5f;
            gParticle[trailIndex].currentTime = 0;
            gParticle[trailIndex].isRandomMove = 0;
            gParticle[trailIndex].isTrailEmit = 0;
            gParticle[trailIndex].isGravity = 0;
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

            if (gParticle[particleIndex].isGravity == 1)
            {
                gParticle[particleIndex].velocity += kGravity * gPerFrame.deltaTime;
            }

                gParticle[particleIndex].prevTranslate = gParticle[particleIndex].translate;
            gParticle[particleIndex].translate += gParticle[particleIndex].velocity * gPerFrame.deltaTime;
            
            if (gParticle[particleIndex].isTrailEmit == 1)
            {
                EmitTrail(particleIndex);
            }
            
            gParticle[particleIndex].currentTime += gPerFrame.deltaTime;
            float lifeRatio = gParticle[particleIndex].currentTime / gParticle[particleIndex].lifeTime;

            float alpha = 1.0f - lifeRatio;
            gParticle[particleIndex].color.a = saturate(alpha);

            gParticle[particleIndex].scale = gParticle[particleIndex].startScale * (1.0f - lifeRatio);

            if (gParticle[particleIndex].color.a == 0.0f)
            {
                gParticle[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);

                // tailを1増やし、古いtail値を取得（atomic +1）
                int oldTail;
                InterlockedAdd(gFreeListTailIndex[0], 1, oldTail);
                // リングバッファ化
                int slot = oldTail % kMaxParticles;
                // 空きスロットにパーティクル番号を保存
                gFreeList[slot] = particleIndex;
            }
        }
    }
}