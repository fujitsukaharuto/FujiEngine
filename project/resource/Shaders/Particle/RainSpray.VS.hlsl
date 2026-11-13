#define RAIN_LIGHTS
#include "Rain.hlsli"

// 走る車のタイヤが跳ね上げる水。粒は今のタイヤの位置から速さ×経過時間だけ戻った所で生まれる
// 大半は細かい水滴の筋、残りは水煙 (太くて薄い)

StructuredBuffer<RainSprayWheel> gRainSprayWheels : register(t0, space3);

// RainRenderer.h の kSprayParticles_ と同じ値であること
static const uint kSprayParticles = 96;
static const float kSprayGravity = 9.8f;

RainVertexOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    RainVertexOutput output = (RainVertexOutput) 0;
    output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);

    uint wheelIndex = instanceID / kSprayParticles;
    uint particle = instanceID % kSprayParticles;
    if (wheelIndex >= sprayWheelCount)
    {
        return output;
    }
    RainSprayWheel wheel = gRainSprayWheels[wheelIndex];
    float3 carVelocity = wheel.velocity.xyz;
    float carSpeed = length(carVelocity.xz);
    if (carSpeed < 1.0f)
    {
        return output;
    }
    float3 forward = float3(carVelocity.x, 0.0f, carVelocity.z) / carSpeed;
    float3 right = float3(forward.z, 0.0f, -forward.x);
    // 速いほど高く・多く跳ねる
    float strength = saturate(carSpeed / 12.0f);

    float3 seed0 = RainHash3(wheelIndex * 977u + particle * 13u + 7u);
    bool isMist = seed0.z < 0.4f;
    float life = isMist ? lerp(0.6f, 1.0f, seed0.x) : lerp(0.25f, 0.45f, seed0.x);
    float period = life * 1.25f;
    float cycleTime = rainTime / period + seed0.y;
    float age = frac(cycleTime) * period;
    if (age > life)
    {
        return output;
    }
    uint cycle = uint(floor(cycleTime));
    float3 r = RainHash3(wheelIndex * 7919u + particle * 31u + cycle * 104729u);
    float3 r2 = RainHash3(wheelIndex * 6151u + particle * 17u + cycle * 7193u + 3u);

    // 生まれた位置: タイヤの後ろ寄りの接地点
    float width = wheel.position.w;
    float3 birth = wheel.position.xyz - carVelocity * age +
                   right * ((r.x - 0.5f) * width) - forward * (0.15f * width);

    // 後ろと外側へ、上へ。水煙はゆっくり広がる
    float sideSign = (r.y < 0.5f) ? -1.0f : 1.0f;
    float3 velocity;
    if (isMist)
    {
        velocity = carVelocity * lerp(0.15f, 0.45f, r.z) +
                   right * (sideSign * lerp(0.1f, 0.6f, r2.x)) +
                   float3(0.0f, lerp(0.3f, 0.9f, r2.y) * strength, 0.0f) -
                   forward * lerp(0.0f, 0.6f, r2.z);
    }
    else
    {
        velocity = carVelocity * lerp(-0.05f, 0.35f, r.z) +
                   right * (sideSign * lerp(0.2f, 1.0f, r2.x) * strength) +
                   float3(0.0f, lerp(0.8f, 2.2f, r2.y) * strength, 0.0f) -
                   forward * (lerp(0.3f, 1.5f, r2.z) * strength);
    }
    // 水煙は空気に止められる
    float drag = isMist ? (1.0f - exp(-age * 2.0f)) / 2.0f : age;
    float gravity = isMist ? 0.05f : 1.0f;
    float3 position = birth + velocity * drag + float3(0.0f, 0.04f - 0.5f * kSprayGravity * gravity * age * age, 0.0f);
    if (position.y < wheel.position.y)
    {
        return output;
    }
    float3 currentVelocity = velocity * (isMist ? exp(-age * 2.0f) : 1.0f) - float3(0.0f, kSprayGravity * gravity * age, 0.0f);
    float speed = length(currentVelocity);
    float3 dir = currentVelocity / max(speed, 1e-3f);

    float3 toCamera = rainCameraPosition - position;
    float distance = length(toCamera);
    float fade = smoothstep(0.5f, 1.5f, distance) * (1.0f - smoothstep(30.0f, 45.0f, distance));
    if (fade <= 0.0f)
    {
        return output;
    }
    float3 side = cross(dir, toCamera / max(distance, 1e-3f));
    side = side / max(length(side), 1e-4f);

    static const uint kCorner[6] = { 0, 1, 2, 2, 1, 3 };
    uint corner = kCorner[vertexID % 6u];
    float acrossSign = (corner & 1) ? 1.0f : -1.0f;
    float alongT = (corner & 2) ? 1.0f : 0.0f;
    float baseWidth = isMist ? lerp(0.08f, 0.18f, r.x) * (0.4f + 1.0f * age / life) : rainWidth * 0.7f;
    float pixelWidth = max(baseWidth, distance * 0.0012f);
    float streakLength = isMist ? baseWidth * 2.0f : 0.015f + speed * 0.02f;
    float3 world = position - dir * (streakLength * (alongT - (isMist ? 0.5f : 0.0f))) + side * (pixelWidth * acrossSign);
    output.position = mul(float4(world, 1.0f), rainViewProjection);
    output.across = acrossSign;
    // 水煙は丸くぼかす (Rain.PS は along が 2 以上を丸く描く)
    output.along = isMist ? 2.0f + alongT : alongT;

    float lifeFade = 1.0f - age / life;
    float density = isMist ? 0.4f * lifeFade * saturate(age / 0.1f) : lifeFade;
    float3 light = RainDropLight(position, toCamera / max(distance, 1e-3f), output.position);
    output.color = RainSoftClamp(light * (rainBrightness * sprayBrightness)) * (fade * density * strength * baseWidth / pixelWidth);
    return output;
}
