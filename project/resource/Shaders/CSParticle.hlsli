struct VertxShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct DrawIndexedArgs
{
    uint IndexCountPerInstance; // 6
    uint InstanceCount; // 生存数
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};

static const float3 kGravity = float3(0.0f, -8.0f, 0.0f);
// ※この値は numParticles (GPUParticleSystem.h) と必ず一致させること
static const uint kMaxParticles = 50331648;
// 位置パッキング用ワールド半径 (±R)。この範囲外は saturate でクランプされる。
// R=512 で刻み 2R/(2^21-1)≒0.0005u、静的誤差±0.00024u。クリップ/フリーズが出たら調整。
static const float kPosPackRadius = 512.0f;
// 2D Dispatch用: 1行あたりのスレッド数 (1024グループ * 1024スレッド)
static const uint kThreadsPerRow = 1024 * 1024;

// 2D Dispatchから1Dインデックスを計算する関数
uint GetParticleIndex(uint3 DTid)
{
    return DTid.x + DTid.y * kThreadsPerRow;
}

struct Particle
{
    float3 translate;

    float scale;
    float startScale;

    float lifeTime;
    float3 velocity;
    float currentTime;
    uint color; // RGBA8_UNORM

    float3 prevTranslate;
    uint flags; // bitfield
};

struct Particle_Translate
{
    uint2 packedPos; // 各軸21bit固定小数(x:bit0-20, y:bit21-41, z:bit42-62)。PackPos21/UnpackPos21 で R/W。
};

struct Particle_Scale
{
    uint packedScale; // f32tof16 packed (scale, startScale)
};

struct Particle_Time
{
    float lifeTime;
    float currentTime;
};

struct Particle_Velocity
{
    uint2 packedVelocity; // f32tof16 packed (x, y, z)
};

struct Particle_Color
{
    uint color;
};

struct Particle_Flags
{
    uint flags;
};

// f32tof16 2つを uint にパッキングする関数
uint PackHalf2(float2 v)
{
    uint2 packed = f32tof16(v);
    return (packed.x & 0xFFFF) | (packed.y << 16);
}

// uint から f16tof32 2つをアンパッキングする関数
float2 UnpackHalf2(uint packed)
{
    return f16tof32(uint2(packed & 0xFFFF, packed >> 16));
}

// f32tof16 3つを uint2 にパッキングする関数
uint2 PackHalf3(float3 v)
{
    uint3 packed = f32tof16(v);
    return uint2((packed.x & 0xFFFF) | (packed.y << 16), packed.z & 0xFFFF);
}

// uint2 から f16tof32 3つをアンパッキングする関数
float3 UnpackHalf3(uint2 packed)
{
    float2 xy = f16tof32(uint2(packed.x & 0xFFFF, packed.x >> 16));
    float z = f16tof32(packed.y & 0xFFFF);
    return float3(xy, z);
}

// 位置 (±kPosPackRadius) を各軸21bit固定小数で uint2 にパッキングする関数。
// レイアウト: lo = qx(bit0-20) | qy下位11bit(bit21-31), hi = qy上位10bit(bit0-9) | qz(bit10-30)。
// fp16絶対座標と違い刻みが座標値に依らず一定なので遅い粒子もフリーズしない。
static const float kPosQuantMax = 2097151.0f; // (1<<21)-1
uint2 PackPos21(float3 p)
{
    float3 n = saturate(p / kPosPackRadius * 0.5f + 0.5f); // [0,1] (範囲外はクランプ)
    uint3 q = (uint3) (n * kPosQuantMax + 0.5f);
    uint lo = q.x | (q.y << 21);            // qy上位10bitは32bit境界で自然に切り捨て
    uint hi = (q.y >> 11) | (q.z << 10);
    return uint2(lo, hi);
}

// uint2 から位置をアンパッキングする関数
float3 UnpackPos21(uint2 packed)
{
    uint qx = packed.x & 0x1FFFFF;
    uint qy = ((packed.x >> 21) & 0x7FF) | ((packed.y & 0x3FF) << 11);
    uint qz = (packed.y >> 10) & 0x1FFFFF;
    float3 q = float3(qx, qy, qz);
    return (q / kPosQuantMax * 2.0f - 1.0f) * kPosPackRadius;
}


// RGBA8_UNORM パッキング関数
uint PackRGBA8(float4 color)
{
    uint4 c = uint4(saturate(color) * 255.0f + 0.5f);
    return (c.a << 24) | (c.b << 16) | (c.g << 8) | c.r;
}

// RGBA8_UNORM アンパッキング関数
float4 UnpackRGBA8(uint packed)
{
    return float4(
        (packed & 0xFF),
        (packed >> 8) & 0xFF,
        (packed >> 16) & 0xFF,
        (packed >> 24) & 0xFF
    ) / 255.0f;
}