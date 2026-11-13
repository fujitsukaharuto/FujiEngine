#define RAIN_LIGHTS
#include "Rain.hlsli"

// 雨だれ。RainDripFind.CS が見つけた場所から、間をあけて滴が落ちる。1か所に滴2つ (各6頂点)

StructuredBuffer<RainDripSite> gRainDripSites : register(t0, space3);

static const uint kDripDropsPerSite = 2;
static const float kDripGravity = 9.8f;

RainVertexOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    RainVertexOutput output = (RainVertexOutput) 0;
    output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);

    RainDripSite site = gRainDripSites[instanceID];
    // 縁の全部からは垂らさず、ところどころにする
    if (site.state != 2u || site.seed < 1.0f - 0.7f * dripAmount)
    {
        return output;
    }
    float top = site.site.y;
    float land = site.site.w;
    float fallTime = sqrt(2.0f * (top - land) / kDripGravity);

    // 滴ごとに間隔をずらす。1周期は「縁で滴がふくらむ間」と「落ちる間」
    uint drop = vertexID / 6u;
    float3 random = RainHash3(uint(site.seed * 16777216.0f) * 5u + drop);
    float period = lerp(0.5f, 1.8f, random.x) + fallTime;
    float hangTime = period - fallTime;
    float cycleAge = frac(rainTime / period + random.y) * period;
    // ふくらむ滴は1か所に1つだけ見せる
    bool isHanging = cycleAge < hangTime;
    if (isHanging && drop != 0u)
    {
        return output;
    }
    float grow = isHanging ? cycleAge / hangTime : 1.0f;
    float age = isHanging ? 0.0f : cycleAge - hangTime;
    float speed = kDripGravity * age;
    float3 position = float3(site.site.x, top - 0.004f - 0.5f * kDripGravity * age * age, site.site.z);
    // 風で少し流す
    position.xz += rainWind * (age * 0.15f);

    float3 toCamera = rainCameraPosition - position;
    float distance = length(toCamera);
    float fade = smoothstep(0.6f, 1.5f, distance) * (1.0f - smoothstep(16.0f, 22.0f, distance));
    if (fade <= 0.0f)
    {
        return output;
    }
    float3 dir = float3(0.0f, -1.0f, 0.0f);
    float3 side = cross(dir, toCamera / max(distance, 1e-3f));
    side = side / max(length(side), 1e-4f);

    static const uint kCorner[6] = { 0, 1, 2, 2, 1, 3 };
    uint corner = kCorner[vertexID % 6u];
    float acrossSign = (corner & 1) ? 1.0f : -1.0f;
    float alongT = (corner & 2) ? 1.0f : 0.0f;
    // 雨粒より太い。遠くは1画素の太さまで広げて薄くする
    float baseWidth = rainWidth * 1.1f * (isHanging ? lerp(0.5f, 1.0f, grow) : 1.0f);
    float width = max(baseWidth, distance * 0.0012f);
    // 速くなるほど伸びる (ぶれ)。ふくらむ滴は丸い粒
    float streakLength = isHanging ? width * 2.2f : 0.025f + speed * 0.018f;
    float3 world = position - dir * (streakLength * alongT) + side * (width * acrossSign);
    output.position = mul(float4(world, 1.0f), rainViewProjection);
    output.across = acrossSign;
    output.along = alongT;

    // ふくらむ滴は小さく光る点から
    float appear = isHanging ? 0.35f + 0.65f * grow * grow : 1.0f;
    float3 light = RainDropLight(position, toCamera / max(distance, 1e-3f), output.position);
    output.color = RainSoftClamp(light * (rainBrightness * dripBrightness)) * (fade * appear * baseWidth / width);
    return output;
}
