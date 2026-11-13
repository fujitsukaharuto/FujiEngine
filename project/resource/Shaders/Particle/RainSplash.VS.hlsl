#define RAIN_LIGHTS
#include "Rain.hlsli"

// 雨のしぶき。水柱1つと水滴4つ (各6頂点)
// 落ちる場所は真上からのレイで求める (路面・車の屋根・庇)

RaytracingAccelerationStructure gSceneTLAS : register(t0, space2);

static const uint kSplashDroplets = 5;
static const float kSplashLife = 0.2f;
// 水柱は先に消える
static const float kCrownLife = 0.09f;
static const float kGravity = 9.8f;
// インスタンスの InstanceMask。RayTracedShadow.hlsli の kRayMaskOpaque と同じ値
static const uint kSplashRayMask = 0x01;

/// <summary>真上から落とした雨粒が最初に当たる高さ。当たらなければ false</summary>
bool FindRainHit(float2 xz, out float height)
{
    RayDesc ray;
    ray.Origin = float3(xz.x, rainCameraPosition.y + 300.0f, xz.y);
    ray.Direction = float3(0.0f, -1.0f, 0.0f);
    ray.TMin = 0.0f;
    ray.TMax = 600.0f;
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kSplashRayMask, ray);
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            q.CommitNonOpaqueTriangleHit();
        }
    }
    height = ray.Origin.y - q.CommittedRayT();
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

RainVertexOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    RainVertexOutput output = (RainVertexOutput) 0;
    output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);

    // しぶきごとに周期と位相を変え、周期ごとに別の場所へ出す
    float3 slot = RainHash3(instanceID * 3u + 1u);
    float period = lerp(0.3f, 0.6f, slot.x);
    float cycleTime = rainTime / period + slot.y;
    float age = frac(cycleTime) * period;
    if (age > kSplashLife)
    {
        return output;
    }
    uint cycle = uint(floor(cycleTime));
    float3 seed = RainHash3(instanceID ^ (cycle * 0x9E3779B9u));
    float2 xz = splashCenter + (seed.xy - 0.5f) * splashArea;

    float3 toCamera0 = rainCameraPosition - float3(xz.x, rainCameraPosition.y, xz.y);
    float flatDistance = length(toCamera0.xz);
    float fade = smoothstep(0.5f, 2.0f, flatDistance) * (1.0f - smoothstep(splashArea * 0.35f, splashArea * 0.5f, flatDistance));
    if (fade <= 0.0f)
    {
        return output;
    }
    float height;
    if (!FindRainHit(xz, height))
    {
        return output;
    }

    // 水滴ごとの飛ぶ向きと速さ。しぶきごとに大きさを変え、水滴もいくつか間引く
    uint droplet = vertexID / 6u;
    float3 dropletRandom = RainHash3(instanceID * 7u + droplet + cycle * 131u);
    float3 sizeRandom = RainHash3(instanceID * 11u + cycle * 977u);
    float splashScale = lerp(0.45f, 1.15f, sizeRandom.x);
    bool isCrown = droplet == 0u;
    if ((isCrown && age > kCrownLife) || (!isCrown && dropletRandom.x < 0.3f))
    {
        return output;
    }
    float angle = 6.2831853f * (float(droplet) + seed.z + dropletRandom.x) / float(kSplashDroplets - 1u);
    float horizontalSpeed = isCrown ? 0.0f : lerp(0.25f, 0.9f, dropletRandom.y) * splashScale;
    float verticalSpeed = (isCrown ? lerp(0.9f, 1.3f, dropletRandom.z) : lerp(0.5f, 1.3f, dropletRandom.z)) * splashScale;
    float2 flatDir = float2(cos(angle), sin(angle));
    float3 position = float3(xz.x, height + 0.01f, xz.y) +
                      float3(flatDir.x * horizontalSpeed, verticalSpeed, flatDir.y * horizontalSpeed) * age +
                      float3(0.0f, -0.5f * kGravity * age * age, 0.0f);
    if (position.y < height)
    {
        return output;
    }
    float3 velocity = float3(flatDir.x * horizontalSpeed, verticalSpeed - kGravity * age, flatDir.y * horizontalSpeed);
    float speed = length(velocity);
    float3 dir = velocity / max(speed, 1e-3f);

    float3 toCamera = rainCameraPosition - position;
    float distance = length(toCamera);
    float3 side = cross(dir, toCamera / max(distance, 1e-3f));
    side = side / max(length(side), 1e-4f);

    static const uint kCorner[6] = { 0, 1, 2, 2, 1, 3 };
    uint corner = kCorner[vertexID % 6u];
    float acrossSign = (corner & 1) ? 1.0f : -1.0f;
    float alongT = (corner & 2) ? 1.0f : 0.0f;
    float baseWidth = rainWidth * 0.7f;
    float width = max(baseWidth, distance * 0.0012f);
    float streakLength = isCrown ? 0.035f * splashScale + speed * 0.012f : 0.006f + speed * 0.012f;
    float3 world = position - dir * (streakLength * alongT) + side * (width * acrossSign);
    output.position = mul(float4(world, 1.0f), rainViewProjection);
    output.across = acrossSign;
    output.along = alongT;

    float life = 1.0f - age / (isCrown ? kCrownLife : kSplashLife);
    float3 light = RainDropLight(position, toCamera / max(distance, 1e-3f), output.position);
    output.color = RainSoftClamp(light * (rainBrightness * splashBrightness)) * (fade * life * baseWidth / width);
    return output;
}
