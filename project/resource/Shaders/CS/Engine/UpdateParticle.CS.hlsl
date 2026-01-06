#include "../../CSParticle.hlsli"
#include "../Noise.hlsli"

RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u0);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u1);
RWStructuredBuffer<Particle_Time> gParticles_Time : register(u2);
RWStructuredBuffer<Particle_Velocity> gParticles_Velocity : register(u3);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u4);
RWStructuredBuffer<Particle_Flags> gParticles_Flags : register(u5);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b0);
RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

void MoveMode(uint pIndex)
{
    if (gParticles_Flags[pIndex].isRandomMove == 1)
    {
        float3 pos = gParticles_Trans[pIndex].translate;
        float time = gPerFrame.time;

        float3 samplePos = pos * 0.4 + float3(0, time * 0.5 + frac(pIndex * 0.012) * 10.0, 0);// pIndexが大きくなった時に値がでかくなりすぎないように
        float3 curl = CurlNoise(samplePos);

        float3 vel0 = gParticles_Velocity[pIndex].velocity;
        float len0 = length(vel0);
        if (len0 < 0.0001f)
        {
            float3 seed = pos * 0.3 + float3(1.234, 5.678, 9.1011);
            float3 rnd = CurlNoise(seed);
            float3 v = rnd + float3(0.1, 0.2, 0.1);
            float l = length(v);
            if (l < 0.0001f)
            {
                v = float3(0.3, 0.1, 0.2);
                l = length(v);
            }
            vel0 = v / l * 0.005f;
            len0 = 0.005f;
        }
        float baseSpeed = max(len0, 0.01f);
        float baseLen = length(vel0);
        float3 baseDir;
        if (baseLen < 0.0001f)
        {
            baseDir = float3(0.0f, 1.0f, 0.0f);
        }
        else
        {
            baseDir = vel0 / baseLen;
        }
        float3 force = curl * 0.1;// ノイズに強度をつける
        float3 vel = baseDir + force;
        float vlen = length(vel);
        if (vlen < 0.0001f)
        {
            vel = baseDir;
            vlen = length(vel);
            if (vlen < 0.0001f)
            {
                vel = float3(0.0f, 1.0f, 0.0f);
                vlen = 1.0f;
            }
        }

        vel /= vlen;
        gParticles_Velocity[pIndex].velocity = vel * baseSpeed;
    }
    else if (gParticles_Flags[pIndex].isRandomMove == 2)
    {
        float3 pos = gParticles_Trans[pIndex].translate;
        float time = gPerFrame.time;
        float3 samplePos =pos * 0.5f +
        float3(0.0f, time * 0.8f, 0.0f);

        float3 curl = CurlNoise(samplePos);

        float noisePower = 4.0f; // ノイズ強度
        float speed = length(gParticles_Velocity[pIndex].velocity);

        gParticles_Velocity[pIndex].velocity =
        normalize(gParticles_Velocity[pIndex].velocity + curl * noisePower) * speed;
    }

}

void EmitTrail(uint pIndex)
{
    float dist = length(gParticles_Trans[pIndex].translate - gParticles_Trans[pIndex].prevTranslate);
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
            float3 trailPos = lerp(gParticles_Trans[pIndex].prevTranslate,gParticles_Trans[pIndex].translate,k);

            gParticles_Trans[trailIndex].translate = trailPos;
            gParticles_Trans[trailIndex].prevTranslate = trailPos;
            gParticles_Scale[trailIndex].scale = gParticles_Scale[pIndex].scale * 0.75f;
            gParticles_Scale[trailIndex].startScale = gParticles_Scale[pIndex].scale * 0.75f;
            gParticles_Time[trailIndex].lifeTime = gParticles_Time[pIndex].lifeTime * 0.5f;
            gParticles_Time[trailIndex].currentTime = 0;
            gParticles_Velocity[trailIndex].velocity = float3(0, 0, 0);
            gParticles_Color[trailIndex].color = gParticles_Color[pIndex].color;
            gParticles_Color[trailIndex].color.a = 1.0f;
            gParticles_Flags[trailIndex].isRandomMove = 0;
            gParticles_Flags[trailIndex].isTrailEmit = 0;
            gParticles_Flags[trailIndex].isGravity = 0;
        }
    }
}

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        if (gParticles_Color[particleIndex].color.a != 0)
        {
            if (gParticles_Flags[particleIndex].isRandomMove != 0)
            {
                MoveMode(particleIndex);
            }

            if (gParticles_Flags[particleIndex].isGravity == 1)
            {
                gParticles_Velocity[particleIndex].velocity += kGravity * gPerFrame.deltaTime;
            }

            gParticles_Trans[particleIndex].prevTranslate = gParticles_Trans[particleIndex].translate;
            gParticles_Trans[particleIndex].translate += gParticles_Velocity[particleIndex].velocity * gPerFrame.deltaTime;
            
            if (gParticles_Flags[particleIndex].isTrailEmit == 1)
            {
                EmitTrail(particleIndex);
            }
            
            gParticles_Time[particleIndex].currentTime += gPerFrame.deltaTime;
            float lifeRatio = gParticles_Time[particleIndex].currentTime / gParticles_Time[particleIndex].lifeTime;

            float alpha = 1.0f - lifeRatio;
            gParticles_Color[particleIndex].color.a = saturate(alpha);

            gParticles_Scale[particleIndex].scale = gParticles_Scale[particleIndex].startScale * (1.0f - lifeRatio);

            if (gParticles_Color[particleIndex].color.a == 0.0f)
            {
                gParticles_Scale[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);

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