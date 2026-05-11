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
    uint yOffset;
    float padding1;
    float4 frustumPlanes[6]; // 視錐台の6平面
};
ConstantBuffer<PerFrame> gPerFrame : register(b0);

// パーティクルが視錐台内にあるかチェックする関数
bool IsVisible(float3 pos, float scale)
{
    // 各平面に対してチェック
    for (int i = 0; i < 6; ++i)
    {
        // 平面の方程式: ax + by + cz + d = 0
        // dot(plane.xyz, pos) + plane.w が 0以上なら平面の内側
        // パーティクルの半径（scale）も考慮してマージンを持たせる
        if (dot(gPerFrame.frustumPlanes[i].xyz, pos) + gPerFrame.frustumPlanes[i].w < -scale)
        {
            return false; // 完全に外側
        }
    }
    return true;
}

RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

RWStructuredBuffer<DrawIndexedArgs> gDrawArgs : register(u9);
RWStructuredBuffer<uint> gDrawParticleIndex : register(u10);

void MoveMode(uint pIndex, uint isRandomMove)
{
    if (isRandomMove == 1)
    {
        float3 pos = gParticles_Trans[pIndex].translate;
        float time = gPerFrame.time;

        float3 samplePos = pos * 0.4 + float3(0, time * 0.5 + frac(pIndex * 0.012) * 10.0, 0);
        float3 curl = CurlNoise(samplePos);

        float3 vel0 = UnpackHalf3(gParticles_Velocity[pIndex].packedVelocity);
        float len0 = length(vel0);
        if (len0 < 0.0001f)
        {
            float3 v = curl + float3(0.1, 0.2, 0.1);
            float l = length(v);
            if (l < 0.0001f) { v = float3(0.3, 0.1, 0.2); l = 1.0f; }
            vel0 = v / l * 0.005f;
            len0 = 0.005f;
        }
        float baseSpeed = max(len0, 0.01f);
        float3 baseDir = normalize(vel0);
        float3 force = curl * 0.1;
        float3 vel = normalize(baseDir + force);

        gParticles_Velocity[pIndex].packedVelocity = PackHalf3(vel * baseSpeed);
    }
    else if (isRandomMove == 2)
    {
        float3 pos = gParticles_Trans[pIndex].translate;
        float time = gPerFrame.time;
        float3 samplePos = pos * 0.5f + float3(0.0f, time * 0.8f, 0.0f);
        float3 curl = CurlNoise(samplePos);

        float3 velocity = UnpackHalf3(gParticles_Velocity[pIndex].packedVelocity);
        float noisePower = 4.0f;
        float speed = length(velocity);
        gParticles_Velocity[pIndex].packedVelocity = PackHalf3(normalize(velocity + curl * noisePower) * speed);
    }
}

void EmitTrail(uint pIndex, uint colorUint, float scale)
{
    float3 pos = gParticles_Trans[pIndex].translate;
    float3 prevPos = gParticles_Trans[pIndex].prevTranslate;
    float dist = length(pos - prevPos);
    if (dist > 0.01f)
    {
        int numTrail = clamp((int)(dist * 100.0f), 0, 60);
        if (numTrail == 0) return;

        uint originalHead;
        InterlockedAdd(gFreeListIndex[0], numTrail, originalHead);
        uint capacity = kMaxParticles;
        uint tail = gFreeListTailIndex[0];
        if (originalHead + numTrail >= tail)
        {
            InterlockedAdd(gFreeListIndex[0], -numTrail);
            return;
        }

        for (int t = 0; t < numTrail; ++t)
        {
            int slot = (originalHead + t) % capacity;
            uint trailIndex = gFreeList[slot];

            float k = (float)t / (float)numTrail;
            float3 trailPos = lerp(prevPos, pos, k);

            gParticles_Trans[trailIndex].translate = trailPos;
            gParticles_Trans[trailIndex].prevTranslate = trailPos;
            gParticles_Scale[trailIndex].packedScale = PackHalf2(float2(scale * 0.75f, scale * 0.75f));
            gParticles_Time[trailIndex].lifeTime = gParticles_Time[pIndex].lifeTime * 0.5f;
            gParticles_Time[trailIndex].currentTime = 0;
            gParticles_Velocity[trailIndex].packedVelocity = PackHalf3(float3(0, 0, 0));
            gParticles_Color[trailIndex].color = colorUint;
            gParticles_Flags[trailIndex].flags = 0;
        }
    }
}

uint GetParticleIndexWithOffset(uint3 DTid)
{
    return DTid.x + (DTid.y + gPerFrame.yOffset) * kThreadsPerRow;
}

groupshared uint gs_AliveCount;
groupshared uint gs_WriteOffset;
groupshared uint gs_DeadCount;
groupshared uint gs_DeadOffset;

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex )
{
    // グループ共有カウンタの初期化
    if (GI == 0) {
        gs_AliveCount = 0;
        gs_DeadCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint particleIndex = GetParticleIndexWithOffset(DTid);
    bool isVisible = false;
    bool isDead = false;
    uint localAliveIndex = 0;
    uint localDeadIndex = 0;

    if (particleIndex < kMaxParticles)
    {
        uint colorUint = gParticles_Color[particleIndex].color;
        float4 color = UnpackRGBA8(colorUint);
        
        // 生きているパーティクルのみ処理
        if (color.a > 0.0f)
        {
            uint flags = gParticles_Flags[particleIndex].flags;
            uint isRandomMove = flags & 0x3;
            uint isTrailEmit = (flags >> 2) & 0x1;
            uint isGravity = (flags >> 3) & 0x1;

            if (isRandomMove != 0) MoveMode(particleIndex, isRandomMove);
            
            float3 velocity = UnpackHalf3(gParticles_Velocity[particleIndex].packedVelocity);
            if (isGravity == 1) velocity += kGravity * gPerFrame.deltaTime;

            float3 pos = gParticles_Trans[particleIndex].translate;
            gParticles_Trans[particleIndex].prevTranslate = pos;
            pos += velocity * gPerFrame.deltaTime;
            gParticles_Trans[particleIndex].translate = pos;
            gParticles_Velocity[particleIndex].packedVelocity = PackHalf3(velocity);
            
            float2 scales = UnpackHalf2(gParticles_Scale[particleIndex].packedScale);
            float currentScale = scales.x;
            float startScale = scales.y;

            // 注意: EmitTrail内のアトミック操作は非常に重いため、数千万単位では慎重に使用する必要があります
            if (isTrailEmit == 1) EmitTrail(particleIndex, colorUint, currentScale);
            
            gParticles_Time[particleIndex].currentTime += gPerFrame.deltaTime;
            float lifeRatio = saturate(gParticles_Time[particleIndex].currentTime / gParticles_Time[particleIndex].lifeTime);

            color.a = saturate(1.0f - lifeRatio);
            currentScale = startScale * (1.0f - lifeRatio);

            uint nextColor = PackRGBA8(color);
            
            // 死亡判定
            if ((nextColor >> 24) == 0 || lifeRatio >= 1.0f)
            {
                gParticles_Scale[particleIndex].packedScale = 0;
                gParticles_Color[particleIndex].color = 0;
                isDead = true;
                InterlockedAdd(gs_DeadCount, 1, localDeadIndex);
            }
            else
            {
                gParticles_Color[particleIndex].color = nextColor;
                gParticles_Scale[particleIndex].packedScale = PackHalf2(float2(currentScale, startScale));
                
                //更新と同時に描画用インデックスバッファに登録
                // カリング判定（画面内にいるか）を追加
                if (color.a >= 0.15f && IsVisible(pos, startScale))
                {
                    isVisible = true;
                    InterlockedAdd(gs_AliveCount, 1, localAliveIndex);
                }
            }
        }
    }

    // グループ内の集計結果をグローバルに反映
    GroupMemoryBarrierWithGroupSync();
    if (GI == 0)
    {
        if (gs_AliveCount > 0) InterlockedAdd(gDrawArgs[0].InstanceCount, gs_AliveCount, gs_WriteOffset);
        if (gs_DeadCount > 0) InterlockedAdd(gFreeListTailIndex[0], gs_DeadCount, gs_DeadOffset);
    }
    GroupMemoryBarrierWithGroupSync();

    // グローバルな書き込み位置を計算して登録
    if (isVisible)
    {
        gDrawParticleIndex[gs_WriteOffset + localAliveIndex] = particleIndex;
    }
    
    if (isDead)
    {
        gFreeList[(gs_DeadOffset + localDeadIndex) % kMaxParticles] = particleIndex;
    }
}