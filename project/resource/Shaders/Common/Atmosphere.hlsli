#ifndef ATMOSPHERE_HLSLI
#define ATMOSPHERE_HLSLI

// 太陽の向きから計算する空。SkyBox.cpp の AtmosphereColor と同じ式であること

// AllLights::fog.skyMode の値。C++ 側 (LightManager.h の SkyMode) と同じ値であること
static const uint kSkyModeTexture = 0;
static const uint kSkyModeAtmosphere = 1;

// 真上を見たときの光学的厚さ。レイリー散乱とミー散乱
static const float3 kAtmosphereRayleigh = float3(0.046f, 0.108f, 0.265f);
static const float kAtmosphereMie = 0.015f;
static const float kAtmosphereMieG = 0.6f;
static const float kAtmosphereScale = 18.0f;

/// <summary>その高さの向きに光が通り抜ける割合</summary>
/// <param name="height">向きの上下成分。0が地平線</param>
float3 AtmosphereTransmittance(float height)
{
    float airMass = 1.0f / (max(height, 0.0f) + 0.3f);
    return exp(-(kAtmosphereRayleigh + kAtmosphereMie) * airMass);
}

/// <summary>その向きの空の色。太陽そのものは含まない</summary>
/// <param name="direction">見る向き(単位ベクトル)</param>
/// <param name="sunDirection">太陽へ向かう単位ベクトル</param>
/// <param name="sunColor">太陽の色と強さ</param>
float3 AtmosphereSky(float3 direction, float3 sunDirection, float3 sunColor)
{
    float mu = dot(direction, sunDirection);
    float phaseRayleigh = 3.0f / (16.0f * 3.14159265f) * (1.0f + mu * mu);
    float g = kAtmosphereMieG;
    float phaseMie = (1.0f - g * g) / (4.0f * 3.14159265f * pow(max(1.0f + g * g - 2.0f * g * mu, 1e-4f), 1.5f));

    float3 scatter = (kAtmosphereRayleigh * phaseRayleigh + kAtmosphereMie * phaseMie)
                     / (kAtmosphereRayleigh + kAtmosphereMie);
    float3 inScatter = scatter * (1.0f - AtmosphereTransmittance(direction.y)) * AtmosphereTransmittance(sunDirection.y);

    // 太陽が沈んだら暗くする
    float sunFade = smoothstep(-0.2f, 0.0f, sunDirection.y);
    return sunColor * inScatter * (kAtmosphereScale * sunFade);
}

/// <summary>太陽の円盤。空の色に足す</summary>
float3 AtmosphereSunDisk(float3 direction, float3 sunDirection, float3 sunColor)
{
    float disk = smoothstep(0.9995f, 0.9998f, dot(direction, sunDirection));
    return sunColor * AtmosphereTransmittance(sunDirection.y) * (disk * 30.0f);
}

#endif // ATMOSPHERE_HLSLI
