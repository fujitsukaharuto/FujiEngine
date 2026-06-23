#include "../../CSParticle.hlsli"
#include "../Noise.hlsli"

RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u0);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u1);
RWStructuredBuffer<Particle_Time> gParticles_Time : register(u2);
RWStructuredBuffer<Particle_Velocity> gParticles_Velocity : register(u3);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u4);
RWStructuredBuffer<Particle_Flags> gParticles_Flags : register(u5);
RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

// 注: このスタンドアロンTrailは現在 Update内の EmitTrailWave に置き換え済みで未ディスパッチ。
// prevTranslate 撤去(T2)に伴い、前フレーム位置は速度から近似復元する(dtはCB未保持のため60fps仮定)。
void EmitTrail(uint pIndex)
{
    float3 pos = UnpackPos21(gParticles_Trans[pIndex].packedPos);
    float3 prevPos = pos - UnpackHalf3(gParticles_Velocity[pIndex].packedVelocity) * (1.0f / 60.0f);
    float dist = length(pos - prevPos);
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

        float2 scales = UnpackHalf2(gParticles_Scale[pIndex].packedScale);
        float currentScale = scales.x;
        
        for (int t = 0; t < numTrail; ++t)
        {
            int slot = (originalHead + t) % capacity;
            uint trailIndex = gFreeList[slot];

            float k = (float) t / max(1, numTrail);
            float3 trailPos = lerp(prevPos, pos, k);

            gParticles_Trans[trailIndex].packedPos = PackPos21(trailPos);
            // prevTranslate 撤去(T2)
            gParticles_Scale[trailIndex].packedScale = PackHalf2(float2(currentScale * 0.75f, currentScale * 0.75f));
            gParticles_Time[trailIndex].lifeTime = gParticles_Time[pIndex].lifeTime * 0.5f;
            gParticles_Time[trailIndex].currentTime = 0;
            gParticles_Velocity[trailIndex].packedVelocity = PackHalf3(float3(0, 0, 0));
            gParticles_Color[trailIndex].color = gParticles_Color[pIndex].color;
            gParticles_Flags[trailIndex].flags = 0;
        }
    }
}

[numthreads(1024, 1, 1)]
            void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        float4 color = UnpackRGBA8(gParticles_Color[particleIndex].color);
        if (color.a != 0)
        {
            uint flags = gParticles_Flags[particleIndex].flags;
            if (((flags >> 2) & 0x1) == 1) // isTrailEmit
            {
                EmitTrail(particleIndex);
            }
        }
    }
}