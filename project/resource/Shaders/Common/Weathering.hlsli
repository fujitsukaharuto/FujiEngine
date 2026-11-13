#ifndef WEATHERING_HLSLI
#define WEATHERING_HLSLI

#include "Wetness.hlsli"

// 壁の汚れ。雨だれの縦筋・地面近くの泥はね・大きなムラを、壁の向きと高さから付ける

static const float3 kGrimeColor = float3(0.4f, 0.37f, 0.33f); // 汚れきった所の色の倍率
static const float kGrimeRoughness = 0.85f;                    // 汚れきった所の粗さ
static const float kStoryHeight = 2.6f;                        // 1階の高さ (m)
static const float kStreakSpacing = 0.45f;                    // 雨だれの列の間隔 (m)
static const float kSplashHeight = 0.9f;                       // 泥はねが届く高さ (m)

/// <summary>汚れの度合い (0〜1)。横を向いた面だけ</summary>
/// <param name="geometricNormal">法線マップ前の法線</param>
float WeatheringAmount(float3 worldPos, float3 geometricNormal)
{
    float wall = saturate(1.0f - abs(geometricNormal.y) * 2.0f);
    if (wall <= 0.0f)
    {
        return 0.0f;
    }
    // 壁に沿った横の位置
    float2 along = normalize(float2(-geometricNormal.z, geometricNormal.x) + 1e-5f);
    float u = dot(worldPos.xz, along);
    float y = worldPos.y;

    // 雨だれ。細い列ごとに、各階の上から垂れて下へ薄れる筋
    float column = u / kStreakSpacing;
    float story = floor(y / kStoryHeight);
    float2 cell = float2(floor(column), story);
    float present = step(0.45f, WetHash(cell));
    float width = lerp(0.12f, 0.4f, WetHash(cell + 7.1f));
    float center = lerp(0.3f, 0.7f, WetHash(cell + 3.3f)) + (WetValueNoise(float2(cell.x * 5.0f, y * 1.5f)) - 0.5f) * 0.15f;
    float across = 1.0f - smoothstep(0.0f, width, abs(frac(column) - center));
    float drop = ((story + 1.0f) * kStoryHeight - y) / (kStoryHeight * lerp(0.35f, 1.0f, WetHash(cell + 1.9f)));
    float streak = present * across * pow(saturate(1.0f - drop), 1.5f);

    float splash = 1.0f - smoothstep(0.0f, kSplashHeight * (0.6f + 0.8f * WetValueNoise(float2(u * 1.5f, 3.1f))), y);
    float blotch = smoothstep(0.3f, 0.9f, WetValueNoise(float2(u * 0.35f + 11.0f, y * 0.35f)));

    return saturate(streak * 0.9f + splash * 0.6f + blotch * 0.25f) * wall;
}

/// <summary>汚れで色を暗く、面を粗くする</summary>
/// <param name="strength">マテリアルの weathering。0なら何もしない</param>
void ApplyWeathering(float3 worldPos, float3 geometricNormal, float strength, inout float3 albedo, inout float roughness)
{
    if (strength <= 0.0f)
    {
        return;
    }
    float dirt = WeatheringAmount(worldPos, geometricNormal) * saturate(strength);
    albedo *= lerp(float3(1.0f, 1.0f, 1.0f), kGrimeColor, dirt);
    roughness = lerp(roughness, max(roughness, kGrimeRoughness), dirt);
}

#endif // WEATHERING_HLSLI
