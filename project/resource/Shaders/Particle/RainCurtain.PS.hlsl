#define RAIN_LIGHTS
#include "Rain.hlsli"
#include "RainCurtain.hlsli"

// 雨の筋の模様。カメラのまわりの円筒に貼り、筋の幅は角度で決める

// 1つの層が拾うライトの上限
static const uint kCurtainMaxLights = 4;
// 筋1本の角度の幅 (ラジアン)
static const float kCurtainColumnAngle = 0.0028f;
// 筋がある割合
static const float kCurtainDensity = 0.3f;

float RainCurtainHash(uint2 v)
{
    v = v * uint2(1664525u, 22695477u) + uint2(1013904223u, 1u);
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v ^= v >> 16u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v ^= v >> 16u;
    return float(v.x & 0xFFFFFFu) / float(0x1000000);
}

float4 main(RainCurtainOutput input) : SV_TARGET0
{
    float4 farPoint = mul(float4(input.ndc, 1.0f, 1.0f), rainInverseViewProjection);
    float3 dir = normalize(farPoint.xyz / farPoint.w - rainCameraPosition);
    float horizontal = length(dir.xz);
    if (horizontal < 0.05f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    uint layer = input.layer;
    float distance = RainCurtainDistance(layer);
    // 円筒の上の点
    float3 position = rainCameraPosition + dir * (distance / horizontal);
    if (position.y < 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // 横は角度、縦は高さ。風の向きに少し傾ける
    float angle = atan2(dir.z, dir.x) + 3.14159265f;
    float2 flatDir = dir.xz / horizontal;
    float windAcross = dot(rainWind, float2(-flatDir.y, flatDir.x));
    angle += position.y * (windAcross / rainFallSpeed) / distance;
    uint columnCount = uint(6.2831853f / kCurtainColumnAngle);
    float column = angle / kCurtainColumnAngle;
    uint columnIndex = uint(column) % columnCount;
    float across = frac(column) * 2.0f - 1.0f;
    float columnRandom = RainCurtainHash(uint2(columnIndex, layer * 7919u + 11u));
    float columnRandom2 = RainCurtainHash(uint2(columnIndex, layer * 104729u + 5u));

    // 筋の長さも角度で揃える
    float streakLength = distance * 0.035f * lerp(0.7f, 1.3f, columnRandom2);
    float fall = rainFallSpeed * lerp(0.85f, 1.15f, columnRandom);
    float v = (position.y + fall * rainTime) / streakLength + columnRandom * 17.0f;
    uint segment = uint(floor(v) + 65536.0f);
    if (RainCurtainHash(uint2(columnIndex * 3u + layer, segment)) > kCurtainDensity)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    // 下が筋の先頭で濃い
    float along = frac(v);
    float shape = (1.0f - across * across) * (0.3f + 0.7f * along);

    float3 toEye = -dir;
    float3 light = RainLightAtPixel(position, toEye, input.position.xy, kCurtainMaxLights);
    // 奥の層ほど薄い。空の高い所は消す
    float layerWeight = 1.0f / (1.0f + float(layer) * 0.6f);
    float heightFade = 1.0f - smoothstep(25.0f, 60.0f, position.y);
    float3 color = RainSoftClamp(light * rainBrightness) * (curtainBrightness * shape * layerWeight * heightFade);
    return float4(color, 0.0f);
}
