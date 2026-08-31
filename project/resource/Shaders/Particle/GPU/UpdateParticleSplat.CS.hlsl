#include "CSParticle.hlsli"
#include "../../Common/Noise.hlsli"

// ============================================================================
// splatモード専用 Update。視錐台カリングと描画リストへの書き込みを持たない。
// ラスタ用の死に処理を落とすため通常版(UpdateParticle.CS.hlsl)とは別PSOにしている
// ============================================================================

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
    float4 frustumPlanes[6]; // splatでは未使用だがCBレイアウト互換のため保持
};
ConstantBuffer<PerFrame> gPerFrame : register(b0);

RWStructuredBuffer<int> gFreeListIndex : register(u6);
RWStructuredBuffer<uint> gFreeList : register(u7);
RWStructuredBuffer<int> gFreeListTailIndex : register(u8);

RWStructuredBuffer<DrawIndexedArgs> gDrawArgs : register(u9); // splatでは InstanceCount を生存数UI用にのみ使用

// プール2枚化(ピンポン): 読み取り元(readBuf=前フレームwriteBuf)。
// 生存判定/前フレーム位置は Prev から読み、書き込みは gParticles_Trans/Color(=writeBuf)へ。
RWStructuredBuffer<Particle_Translate> gParticles_TransPrev : register(u10);
RWStructuredBuffer<Particle_Color> gParticles_ColorPrev : register(u11);

void MoveMode(uint pIndex, uint isRandomMove)
{
    if (isRandomMove == 1)
    {
        float3 pos = UnpackPos21(gParticles_TransPrev[pIndex].packedPos);
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
        float3 pos = UnpackPos21(gParticles_TransPrev[pIndex].packedPos);
        float time = gPerFrame.time;
        float3 samplePos = pos * 0.5f + float3(0.0f, time * 0.8f, 0.0f);
        float3 curl = CurlNoise(samplePos);
        float3 velocity = UnpackHalf3(gParticles_Velocity[pIndex].packedVelocity);
        float noisePower = 4.0f;
        float speed = length(velocity);
        gParticles_Velocity[pIndex].packedVelocity = PackHalf3(normalize(velocity + curl * noisePower) * speed);
    }
}

// Wave Intrinsics を使用した EmitTrail
// prevTranslate 撤去(T2): 前フレーム位置(prevPos)は呼び出し側(=更新前のtranslate)から受け取る
void EmitTrailWave(uint pIndex, uint colorUint, float scale, float3 pos, float3 prevPos)
{
    float dist = length(pos - prevPos);

    uint numTrail = (dist > 0.01f) ? clamp((uint)(dist * 100.0f), 0, 60) : 0;

    uint waveTotalTrails = WaveActiveSum(numTrail);
    uint wavePrefixOffset = WavePrefixSum(numTrail);
    int waveBaseHead = 0;

    if (WaveIsFirstLane() && waveTotalTrails > 0)
    {
        InterlockedAdd(gFreeListIndex[0], (int)waveTotalTrails, waveBaseHead);

        uint tailVal = (uint)gFreeListTailIndex[0];
        uint headVal = (uint)waveBaseHead;
        uint available = tailVal - headVal;

        if (waveTotalTrails > available)
        {
            InterlockedAdd(gFreeListIndex[0], -(int)waveTotalTrails);
            waveBaseHead = -1;
        }
    }

    waveBaseHead = WaveReadLaneFirst(waveBaseHead);

    if (waveBaseHead != -1 && numTrail > 0)
    {
        uint myStartHead = (uint)waveBaseHead + wavePrefixOffset;
        uint capacity = kMaxParticles;

        for (uint t = 0; t < numTrail; ++t)
        {
            int slot = (int)((myStartHead + t) % capacity);
            uint trailIndex = gFreeList[slot];

            float k = (float)t / (float)numTrail;
            float3 trailPos = lerp(prevPos, pos, k);

            gParticles_Trans[trailIndex].packedPos = PackPos21(trailPos);
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
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    if (GI == 0) {
        gs_AliveCount = 0;
        gs_DeadCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint particleIndex = GetParticleIndexWithOffset(DTid);
    bool isAlive = false;
    bool isDead = false;

    if (particleIndex < kMaxParticles)
    {
        uint colorUint = gParticles_ColorPrev[particleIndex].color;
        float4 color = UnpackRGBA8(colorUint);

        if (color.a > 0.0f)
        {
            uint flags = gParticles_Flags[particleIndex].flags;
            uint isRandomMove = flags & 0x3;
            uint isTrailEmit = (flags >> 2) & 0x1;
            uint isGravity = (flags >> 3) & 0x1;

            if (isRandomMove != 0) MoveMode(particleIndex, isRandomMove);

            float3 velocity = UnpackHalf3(gParticles_Velocity[particleIndex].packedVelocity);
            if (isGravity == 1) velocity += kGravity * gPerFrame.deltaTime;

            float3 prevPos = UnpackPos21(gParticles_TransPrev[particleIndex].packedPos); // 前フレーム位置(readBuf)
            float3 pos = prevPos + velocity * gPerFrame.deltaTime;
            gParticles_Trans[particleIndex].packedPos = PackPos21(pos); // writeBuf へ書き戻し
            gParticles_Velocity[particleIndex].packedVelocity = PackHalf3(velocity);

            float2 scales = UnpackHalf2(gParticles_Scale[particleIndex].packedScale);
            float currentScale = scales.x;
            float startScale = scales.y;

            if (isTrailEmit == 1) EmitTrailWave(particleIndex, colorUint, currentScale, pos, prevPos);

            gParticles_Time[particleIndex].currentTime += gPerFrame.deltaTime;
            float lifeRatio = saturate(gParticles_Time[particleIndex].currentTime / gParticles_Time[particleIndex].lifeTime);

            color.a = saturate(1.0f - lifeRatio);
            currentScale = startScale * (1.0f - lifeRatio);

            uint nextColor = PackRGBA8(color);

            if ((nextColor >> 24) == 0 || lifeRatio >= 1.0f)
            {
                gParticles_Scale[particleIndex].packedScale = 0;
                gParticles_Color[particleIndex].color = 0;
                isDead = true;
            }
            else
            {
                gParticles_Color[particleIndex].color = nextColor;
                gParticles_Scale[particleIndex].packedScale = PackHalf2(float2(currentScale, startScale));
                isAlive = true; // 視錐台カリングはしない(splat側でNDCカリング)
            }
        }
    }

    // --- 生存数の集計(UI用。per-group atomic 1個) ---
    uint waveAliveCount = WaveActiveCountBits(isAlive);
    if (WaveIsFirstLane() && waveAliveCount > 0) {
        uint dummy;
        InterlockedAdd(gs_AliveCount, waveAliveCount, dummy);
    }

    // --- 死亡 → FreeList返却(通常版と同じ) ---
    uint waveDeadCount = WaveActiveCountBits(isDead);
    uint waveDeadPrefix = WavePrefixCountBits(isDead);
    uint waveDeadBase = 0;
    if (WaveIsFirstLane() && waveDeadCount > 0) {
        InterlockedAdd(gs_DeadCount, waveDeadCount, waveDeadBase);
    }
    uint localDeadIndex = WaveReadLaneFirst(waveDeadBase) + waveDeadPrefix;

    GroupMemoryBarrierWithGroupSync();
    if (GI == 0)
    {
        if (gs_AliveCount > 0) {
            uint tempWriteOffset;
            InterlockedAdd(gDrawArgs[0].InstanceCount, gs_AliveCount, tempWriteOffset);
        }
        if (gs_DeadCount > 0) {
            int tempDeadOffset;
            InterlockedAdd(gFreeListTailIndex[0], (int)gs_DeadCount, tempDeadOffset);
            gs_DeadOffset = (uint)tempDeadOffset;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    if (isDead)
    {
        gFreeList[(gs_DeadOffset + localDeadIndex) % kMaxParticles] = particleIndex;
    }
}
