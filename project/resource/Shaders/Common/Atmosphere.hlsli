#ifndef ATMOSPHERE_HLSLI
#define ATMOSPHERE_HLSLI

// 太陽の向きから計算する空。数表を引く側と作る側 (SkyBox/Atmosphere*.CS.hlsl) の共通部分

// AllLights::fog.skyMode の値。C++ 側 (LightManager.h の SkyMode) と同じ値であること
static const uint kSkyModeTexture = 0;
static const uint kSkyModeAtmosphere = 1;

// 空の見え方の数表 (SkyView) の大きさ。SkyAtmosphere.h の定数と一致していること
static const float2 kSkyViewLutSize = float2(192.0f, 108.0f);

// 太陽の円盤の見かけの半径 (ラジアン)。実際の約0.0047より大きい
static const float kSunDiskRadius = 0.0065f;
// 円盤の明るさ。実際よりかなり抑えてある
static const float kSunDiskRadianceScale = 40.0f;

/// <summary>向きから SkyView の数表の位置。横は方位(一周)、縦は天頂角</summary>
/// <remarks>
/// Hillaire 2020 と同じく、縦は地面の縁 (地平線) のまわりへ texel を寄せる。上半分が空、下半分が地面。
/// 地表では地平線の天頂角が 90度で、以前の仰角の平方根の並びと同じになる
/// </remarks>
/// <param name="horizonZenith">カメラから見た地面の縁の天頂角 (ラジアン)。地表で π/2、高いほど大きい</param>
float2 SkyViewDirectionToUv(float3 direction, float horizonZenith)
{
    float azimuth = atan2(direction.z, direction.x);
    float zenith = acos(clamp(direction.y, -1.0f, 1.0f));
    float u = azimuth / 6.28318530718f + 0.5f;
    float v;
    if (zenith < horizonZenith)
    {
        v = 0.5f + 0.5f * sqrt(saturate(1.0f - zenith / max(horizonZenith, 1e-4f)));
    }
    else
    {
        v = 0.5f - 0.5f * sqrt(saturate((zenith - horizonZenith) / max(3.14159265359f - horizonZenith, 1e-4f)));
    }

    // 縦の端は反対側を混ぜないよう内側へ寄せる
    float halfTexel = 0.5f / kSkyViewLutSize.y;
    return float2(u, clamp(v, halfTexel, 1.0f - halfTexel));
}

/// <summary>SkyViewDirectionToUv の逆</summary>
float3 SkyViewUvToDirection(float2 uv, float horizonZenith)
{
    float azimuth = (uv.x - 0.5f) * 6.28318530718f;
    float zenith;
    if (uv.y >= 0.5f)
    {
        float s = uv.y * 2.0f - 1.0f;
        zenith = horizonZenith * (1.0f - s * s);
    }
    else
    {
        float s = 1.0f - uv.y * 2.0f;
        zenith = horizonZenith + (3.14159265359f - horizonZenith) * s * s;
    }
    float sinZenith = sin(zenith);
    return float3(sinZenith * cos(azimuth), cos(zenith), sinZenith * sin(azimuth));
}

/// <summary>レイリー散乱の位相関数</summary>
float RayleighPhase(float cosTheta)
{
    return 3.0f / (16.0f * 3.14159265359f) * (1.0f + cosTheta * cosTheta);
}

/// <summary>ミー散乱の位相関数 (Cornette-Shanks)</summary>
float MiePhase(float g, float cosTheta)
{
    float g2 = g * g;
    float k = 3.0f / (8.0f * 3.14159265359f) * (1.0f - g2) / (2.0f + g2);
    return k * (1.0f + cosTheta * cosTheta) / pow(max(1.0f + g2 - 2.0f * g * cosTheta, 1e-4f), 1.5f);
}

/// <summary>太陽の円盤。縁が暗くなる(周辺減光)</summary>
/// <param name="sunColor">大気を通り抜けてきた太陽の色と強さ</param>
float3 AtmosphereSunDisk(float3 direction, float3 sunDirection, float3 sunColor)
{
    float cosAngle = dot(direction, sunDirection);
    float angle = acos(clamp(cosAngle, -1.0f, 1.0f));
    float r = angle / kSunDiskRadius;
    if (r >= 1.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    float mu = sqrt(1.0f - r * r);
    float limb = 1.0f - 0.6f * (1.0f - mu);
    // 縁を滑らかにする
    float edge = smoothstep(1.0f, 0.7f, r);
    return sunColor * (kSunDiskRadianceScale * limb * edge);
}

// 雲の層。カメラの真上 kCloudHeight に広がる板
static const float kCloudHeight = 1500.0f;          // m
static const float kCloudScale = 1.0f / 3000.0f;     // いちばん大きい塊の周期が約 3km
static const float2 kCloudWind = float2(9.0f, 3.0f); // m/s
static const float kCloudFadeDistance = 30000.0f;   // これより遠い雲は空に溶ける

// Dave Hoskins の Hash without Sine
float CloudHash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float CloudCorner(float2 cell, float2 offset)
{
    float angle = CloudHash(cell) * 6.28318530718f;
    return dot(float2(cos(angle), sin(angle)), offset);
}

// 勾配ノイズ (0〜1)
float CloudNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * f * (f * (f * 6.0f - 15.0f) + 10.0f);
    float a = CloudCorner(i, f);
    float b = CloudCorner(i + float2(1.0f, 0.0f), f - float2(1.0f, 0.0f));
    float c = CloudCorner(i + float2(0.0f, 1.0f), f - float2(0.0f, 1.0f));
    float d = CloudCorner(i + float2(1.0f, 1.0f), f - float2(1.0f, 1.0f));
    return 0.5f + 0.75f * lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

/// <summary>雲の濃さ (0〜1)。p は模様の空間の位置</summary>
float CloudDensity(float2 p, float coverage)
{
    // 段ごとに回して格子の向きを隠す
    const float2x2 rotate = float2x2(0.8f, -0.6f, 0.6f, 0.8f);
    float sum = 0.0f;
    float amplitude = 0.5f;
    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        sum += amplitude * CloudNoise(p);
        p = mul(rotate, p) * 2.03f + 17.1f;
        amplitude *= 0.5f;
    }
    // coverage が大きいほど低い所まで雲にする
    float threshold = lerp(0.72f, 0.3f, saturate(coverage));
    return saturate((sum - threshold) * 3.5f);
}

/// <summary>雲の色と覆い具合。空の色の上に lerp(sky, rgb, a) で重ねる</summary>
/// <param name="sunColor">雲の高さに届く太陽の色と強さ</param>
/// <param name="skyAmbient">真上の空の色。雲の影の側を照らす</param>
/// <param name="horizonColor">この向きの空の色。遠くの雲がここへ溶ける</param>
/// <param name="origin">見ている位置の xz</param>
float4 CloudLayer(float3 direction, float3 sunDirection, float3 sunColor, float3 skyAmbient, float3 horizonColor,
                  float coverage, float time, float2 origin)
{
    if (coverage <= 0.0f || direction.y <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float distance = kCloudHeight / direction.y;
    float2 position = (origin + direction.xz * distance + kCloudWind * time) * kCloudScale;
    float density = CloudDensity(position, coverage);
    if (density <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // 太陽の側の雲の厚さ。厚いほど底が暗い
    float2 toSun = normalize(sunDirection.xz + 1e-4f) * (0.12f * (1.0f - saturate(sunDirection.y)) + 0.04f);
    float towardSun = CloudDensity(position + toSun, coverage);
    float sunTransmittance = exp(-1.5f * towardSun) * saturate(sunDirection.y * 8.0f + 0.2f);
    // 縁は明るく芯は暗い (Beer-Powder)
    float powder = 1.0f - exp(-4.0f * density);

    // 太陽の向きに近いほど縁が光る
    float cosTheta = dot(direction, sunDirection);
    float phase = lerp(MiePhase(-0.2f, cosTheta), MiePhase(0.75f, cosTheta), 0.4f) * 4.0f * 3.14159265359f;

    float3 sunLight = sunColor * (sunTransmittance * powder * (0.3f + 0.2f * phase));
    float3 ambient = skyAmbient * lerp(1.6f, 0.9f, density);
    float3 color = sunLight + ambient;

    // 地平線へ向かって空の色へ溶かす
    float fade = exp(-distance / kCloudFadeDistance);
    color = lerp(horizonColor, color, fade);
    float alpha = (1.0f - exp(-5.0f * density)) * smoothstep(0.0f, 0.06f, direction.y) * saturate(fade * 1.5f);
    return float4(color, alpha);
}

/// <summary>雲の影。太陽の向きの雲の濃さで光を弱める</summary>
/// <returns>太陽の光に掛ける倍率 (1 で遮らない)</returns>
/// <param name="strength">遮る強さ (0〜1)</param>
float CloudShadow(float3 worldPosition, float3 sunDirection, float coverage, float time, float strength)
{
    if (strength <= 0.0f || coverage <= 0.0f || sunDirection.y <= 0.02f)
    {
        return 1.0f;
    }
    float distance = (kCloudHeight - worldPosition.y) / sunDirection.y;
    float2 position = (worldPosition.xz + sunDirection.xz * distance + kCloudWind * time) * kCloudScale;
    float density = CloudDensity(position, coverage);
    return lerp(1.0f, exp(-4.0f * density), saturate(strength));
}

float StarHash(float3 p)
{
    return frac(sin(dot(p, float3(12.9898f, 78.233f, 37.719f))) * 43758.5453f);
}

/// <summary>夜空の星。空を細かい格子に切って星を置く</summary>
/// <remarks>昼も同じ明るさで描く</remarks>
float3 StarField(float3 direction, float time)
{
    // 星1つが画面でおよそ1画素
    const float kStarCells = 220.0f;
    const float kStarChance = 0.985f;
    float3 p = direction * kStarCells;
    float3 cell = floor(p);
    float h = StarHash(cell);
    if (h < kStarChance)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    float3 starPosition = float3(StarHash(cell + 11.3f), StarHash(cell + 27.1f), StarHash(cell + 51.7f)) * 0.7f + 0.15f;
    float3 offset = (p - cell) - starPosition;
    // 明るい星ほど少ない
    float magnitude = pow((h - kStarChance) / (1.0f - kStarChance), 4.0f);
    float twinkle = 0.85f + 0.15f * sin(time * (2.0f + h * 6.0f) + h * 100.0f);
    float brightness = exp(-dot(offset, offset) / (0.15f * 0.15f)) * (0.15f + magnitude * 3.0f) * twinkle;
    float3 tint = lerp(float3(1.0f, 0.85f, 0.7f), float3(0.75f, 0.85f, 1.0f), StarHash(cell + 3.1f));
    // 地平線の近くは空気で消える
    return tint * brightness * smoothstep(0.0f, 0.15f, direction.y);
}

/// <summary>月の円盤。太陽の反対側に描く。縁ほど少し暗い</summary>
float3 MoonDisk(float3 direction, float3 moonDirection)
{
    const float kMoonRadius = 0.012f;
    float cosAngle = dot(direction, moonDirection);
    float angle = acos(clamp(cosAngle, -1.0f, 1.0f));
    if (angle > kMoonRadius * 1.2f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    float r = saturate(angle / kMoonRadius);
    float limb = sqrt(saturate(1.0f - r * r)) * 0.6f + 0.4f;
    // 表面の濃淡(海)。向きから決まる粗い模様
    float maria = CloudNoise(direction.xz * 400.0f + direction.y * 170.0f);
    float edge = 1.0f - smoothstep(0.9f, 1.0f, r);
    // 夜の露出で表面の濃淡が見える明るさ
    return float3(0.85f, 0.88f, 1.0f) * 0.2f * limb * lerp(0.55f, 1.05f, maria) * edge;
}

#endif // ATMOSPHERE_HLSLI
