#ifndef WETNESS_HLSLI
#define WETNESS_HLSLI

// 濡れた路面 (Lagarde "Water drop 2b")。Object3d.PS と DepthNormal.PS が同じ式を使う

static const float kWetDarken = 0.45f;       // 濡れきった面の色の倍率
static const float kWetRoughness = 0.25f;    // 濡れきった面の粗さの上限
static const float kPuddleDarken = 0.8f;     // 水たまりの底がさらに暗くなる倍率
static const float kPuddleRoughness = 0.04f; // 水たまりの粗さ (水面)
static const float kPuddleFlatness = 0.95f;  // これより上を向いた平らな所だけに水が溜まる

struct WetSurface
{
    float wet;    // 濡れ具合 (0〜1)
    float puddle; // 水たまりの度合い (0〜1)
};

float WetHash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float WetValueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0f - 2.0f * f);
    float a = WetHash(i);
    float b = WetHash(i + float2(1.0f, 0.0f));
    float c = WetHash(i + float2(0.0f, 1.0f));
    float d = WetHash(i + float2(1.0f, 1.0f));
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

/// <summary>水たまりの形。大きな塊に細かい縁の揺らぎを重ねる</summary>
float PuddleNoise(float2 xz, float scale)
{
    float2 p = xz / max(scale, 0.01f);
    return WetValueNoise(p) * 0.6f + WetValueNoise(p * 2.7f + 17.3f) * 0.3f + WetValueNoise(p * 7.1f + 5.1f) * 0.1f;
}

/// <summary>その点の濡れ具合。屋根の下かどうかは見ない</summary>
/// <param name="geometricNormal">形状そのものの法線 (法線マップ前)</param>
/// <param name="wetness">全体の濡れ具合。0なら何もしない</param>
/// <param name="puddleAmount">水たまりの多さ (0〜1)</param>
/// <param name="puddleScale">水たまりの大きさ (m)</param>
WetSurface GetWetSurface(float3 worldPos, float3 geometricNormal, float wetness, float puddleAmount, float puddleScale)
{
    WetSurface surface;
    float up = saturate(geometricNormal.y);
    // 上を向くほど濡れる。壁も少し濡れる
    surface.wet = wetness * lerp(0.35f, 1.0f, smoothstep(0.3f, 0.9f, up));
    surface.puddle = 0.0f;
    if (puddleAmount > 0.0f && up > kPuddleFlatness)
    {
        float threshold = 1.0f - puddleAmount * 0.7f;
        float noise = PuddleNoise(worldPos.xz, puddleScale);
        // 濡れが半分を超えてから溜まり始める
        surface.puddle = smoothstep(threshold - 0.03f, threshold + 0.03f, noise) *
                         saturate((up - kPuddleFlatness) / 0.03f) * smoothstep(0.5f, 1.0f, wetness);
    }
    return surface;
}

/// <summary>濡れた面の色・粗さ・金属・法線にする</summary>
void ApplyWetSurface(WetSurface surface, float3 geometricNormal, inout float3 albedo, inout float roughness,
                     inout float metallic, inout float3 normal)
{
    // 金属は水を吸わないので暗くならない
    albedo *= lerp(1.0f, kWetDarken, surface.wet * (1.0f - metallic));
    roughness = lerp(roughness, min(roughness, kWetRoughness), surface.wet);
    // 水たまりは凹凸が消えて滑らかになる
    albedo *= lerp(1.0f, kPuddleDarken, surface.puddle);
    roughness = lerp(roughness, kPuddleRoughness, surface.puddle);
    metallic = lerp(metallic, 0.0f, surface.puddle);
    normal = normalize(lerp(normal, geometricNormal, surface.puddle));
}

/// <summary>濡れた面の粗さだけ (プリパス用)</summary>
float WetRoughness(WetSurface surface, float roughness)
{
    roughness = lerp(roughness, min(roughness, kWetRoughness), surface.wet);
    return lerp(roughness, kPuddleRoughness, surface.puddle);
}

// 雨の波紋。セルごとに広がる輪を2層重ねる
static const float kRippleCellSize = 0.45f;  // 波紋のセルの大きさ (m)
static const float kRippleStrength = 0.35f;  // 法線を傾ける強さ

/// <summary>雨の波紋で法線を傾ける量 (xz)</summary>
/// <param name="time">秒</param>
float2 RainRippleOffset(float2 xz, float time)
{
    float2 offset = float2(0.0f, 0.0f);
    [unroll]
    for (int layer = 0; layer < 2; layer++)
    {
        float2 p = xz / kRippleCellSize + float2(0.37f, 0.71f) * float(layer);
        float2 cell = floor(p);
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        {
            [unroll]
            for (int dx = -1; dx <= 1; dx++)
            {
                float2 c = cell + float2(dx, dy);
                float h = WetHash(c + 13.1f * float(layer));
                float h2 = WetHash(c + 7.7f + 5.3f * float(layer));
                float2 center = c + float2(h, h2);
                // 広がる時間をセルごとに変える
                float phase = frac(time / (0.8f + h * 0.6f) + h2);
                float2 d = p - center;
                float dist = length(d);
                float x = dist - phase * 0.9f;
                float fade = (1.0f - phase) * (1.0f - phase);
                float wave = sin(x * 28.0f) * exp(-x * x * 80.0f) * fade;
                offset += (dist > 1e-3f ? d / dist : float2(0.0f, 0.0f)) * wave;
            }
        }
    }
    return offset;
}

/// <summary>雨が降っていれば、水たまりと平らな面に波紋を立てる</summary>
/// <param name="rain">降っている雨の強さ (0〜1)</param>
void ApplyRainRipples(WetSurface surface, float3 worldPos, float3 geometricNormal, float rain, float time, inout float3 normal)
{
    float weight = rain * max(surface.puddle, surface.wet * 0.3f) * saturate((geometricNormal.y - 0.9f) / 0.1f);
    if (weight <= 0.0f)
    {
        return;
    }
    float2 offset = RainRippleOffset(worldPos.xz, time) * (kRippleStrength * weight);
    normal = normalize(normal + float3(offset.x, 0.0f, offset.y));
}

#endif // WETNESS_HLSLI
