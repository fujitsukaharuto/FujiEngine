#include "../../CSParticle.hlsli"
#include "../Random.hlsli"

RWStructuredBuffer<Particle> gParticle : register(u0);
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
};
ConstantBuffer<EmitterSphere> gEmitter : register(b0);
struct PerFrame
{
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);


float3 RandomUnitVector(inout RandomGenerator gen)
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

float3 GenerateEmitPosition(uint type,inout RandomGenerator generator)
{
    switch (type)
    {
        case 0: // Sphere
        {
                float3 dir = RandomUnitVector(generator);
                return dir * gEmitter.radius;
            }

        case 1: // Ring（円周上）
        {
                float angle = generator.Generate1d() * 6.28318530718f;
                float3 dir = float3(cos(angle), 0, sin(angle));
                return dir * gEmitter.radius;
            }

        case 2: // Plane（平面上）
        {
                float3 p = float3(
                generator.Generate1d() * 2.0f - 1.0f,
                0.0f,
                generator.Generate1d() * 2.0f - 1.0f
            );
                return p * gEmitter.radius;
            }

        case 3: // Cube（立方体内）
        {
                float3 p = generator.Generate3d(); // [-1,1] each
                return p * gEmitter.radius;
            }

        case 4: // Cone (半球上)
        {
                float3 dir = RandomUnitVector(generator);
                dir.y = abs(dir.y);
                return normalize(dir) * gEmitter.radius;
            }

        case 5: // Cylinder
        {
                float angle = generator.Generate1d() * 6.2831853;
                float r = gEmitter.radius;
                float x = cos(angle) * r;
                float z = sin(angle) * r;
                float y = generator.Generate1d() * 1.0f - 0.5f;
                return float3(x, y, z);
            }

        case 6: // Torus
        {
                float major = gEmitter.radius; // 外側の半径
                float minor = 0.5f; // 内側の厚み
                float u = generator.Generate1d() * 6.2831853;
                float v = generator.Generate1d() * 6.2831853;
                float x = (major + minor * cos(v)) * cos(u);
                float y = minor * sin(v);
                float z = (major + minor * cos(v)) * sin(u);
                return float3(x, y, z);
            }

        default:
        {
                float3 dirDefault = RandomUnitVector(generator);
                return dirDefault * gEmitter.radius;
            }
    }

    // default: sphere
    float3 dirDefault = RandomUnitVector(generator);
    return dirDefault * gEmitter.radius;
}

float3 GenerateEmitVelocity(uint type, float3 localPos, inout RandomGenerator generator)
{
    switch (type)
    {
        case 0: // Random all directions
            return RandomUnitVector(generator);

        case 1: // 中心から位置方向へ
            return normalize(localPos);

        case 2: // 中心へ吸い込む
            return -normalize(localPos);

        case 3: // 円周の接線方向
        {
                float3 p = normalize(float3(localPos.x, 0, localPos.z));
                return float3(-p.z, 0, p.x);
            }

        case 4: // 上方向 + ランダムゆらぎ
        {
                float3 up = float3(0, 1, 0);
                float3 rnd = RandomUnitVector(generator) * 0.2f;
                return normalize(up + rnd);
            }
        case 5:
            return float3(0, 0, 0);
    }

    // default: random
    return RandomUnitVector(generator);
}

float3 RotateVector(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}


int PopFreeListIndex()
{
    int oldTop;
    int newTop;
    int original;

    do
    {
        oldTop = gFreeListIndex[0];
        if (oldTop <= 0)
            return -1;

        newTop = oldTop - 1;

        // InterlockedCompareExchange は out パラメータで元の値を返す
        InterlockedCompareExchange(gFreeListIndex[0], oldTop, newTop, original);

        // original != oldTop の場合は別スレッドが先に書き換えたので再試行
    } while (original != oldTop);

    return newTop; // 取り出したインデックス
}

void PushFreeList(uint particleIndex)
{
    int oldTop;
    int newTop;
    int original;

    do
    {
        oldTop = gFreeListIndex[0];
        newTop = oldTop + 1;

        if (newTop >= kMaxParticles)
            return;

        InterlockedCompareExchange(gFreeListIndex[0], oldTop, newTop, original);

    } while (original != oldTop);

    // 安全に書き込める場所が oldTop に確保されている
    gFreeList[oldTop] = particleIndex;
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

        int freeListIndex = PopFreeListIndex();
        if (0 <= freeListIndex && freeListIndex < kMaxParticles)
        {
            // 補間係数（0〜1）: スレッドIDをcountで割る
            float lerpT;
            if (gEmitter.count > 1)
            {
                lerpT = (float) DTid.x / (gEmitter.count - 1);
            }
            else
            {
                lerpT = 0.0f;
            }

            float3 interpPos = gEmitter.translate;
            if (gEmitter.isDistance == 1)
            {
                interpPos = lerp(gEmitter.prevTranslate, gEmitter.translate, lerpT);
            }

            float3 localPos = GenerateEmitPosition(gEmitter.emitShapeType, generator);
            localPos = RotateVector(localPos, gEmitter.rotation);
            float3 pos = interpPos + localPos;

            uint particleIndex = gFreeList[freeListIndex];
            gParticle[particleIndex].scale = gEmitter.scale;
            gParticle[particleIndex].startScale = gEmitter.scale;
            gParticle[particleIndex].translate = pos;
            gParticle[particleIndex].prevTranslate = pos;

            float3 t = (generator.Generate3d() + 1) * 0.5f;
            gParticle[particleIndex].color.rgb = lerp(gEmitter.colorMin, gEmitter.colorMax, t);
            gParticle[particleIndex].color.a = 1.0f;

            float veloT = generator.Generate1d();
            float speed = lerp(gEmitter.velocityRandMin, gEmitter.velocityRandMax, veloT);
            float3 velDir = GenerateEmitVelocity(gEmitter.emitVeloType, localPos, generator);
            gParticle[particleIndex].velocity = gEmitter.baseVelocity + velDir * speed;

            gParticle[particleIndex].lifeTime = gEmitter.lifeTime;
            gParticle[particleIndex].currentTime = 0.0f;
            gParticle[particleIndex].isRandomMove = gEmitter.isRandomMove;
            gParticle[particleIndex].isTrailEmit = gEmitter.isTrailEmit;
        }
    }
}