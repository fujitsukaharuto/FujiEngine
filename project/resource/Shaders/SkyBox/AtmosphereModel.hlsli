#ifndef ATMOSPHERE_MODEL_HLSLI
#define ATMOSPHERE_MODEL_HLSLI

// 大気の物理モデル (Hillaire 2020)。数表を作る側だけが使う
// 長さは km、星の中心が原点で、地表のカメラは真上(+Y)にいる
#include "../Common/Atmosphere.hlsli"

// SkyAtmosphere.h の AtmosphereParams と並びが一致していること
cbuffer AtmosphereParams : register(b0)
{
    float bottomRadius;           // 地表の半径
    float topRadius;              // 大気の上端の半径
    float rayleighDensityScale;   // 高さ1kmごとの濃さの減り方 (-1/スケールハイト)
    float mieDensityScale;

    float3 rayleighScattering;    // 地表での散乱係数 (/km)
    float miePhaseG;              // ミー散乱の前方への偏り

    float3 mieScattering;
    float ozoneCenter;            // オゾン層の中心の高さ

    float3 mieExtinction;
    float ozoneWidth;             // オゾン層の厚みの半分

    float3 ozoneAbsorption;
    float cameraHeight;           // 地表からのカメラの高さ

    float3 groundAlbedo;
    float cloudCoverage;          // 雲の量。0で雲を出さない

    float3 sunDirection;          // 太陽へ向かう単位ベクトル
    float cloudTime;              // 雲を流す時刻(秒)

    float3 sunIlluminance;        // 太陽の色と強さ
    float atmospherePad2;
};

static const float kAtmospherePI = 3.14159265359f;

// 数表の大きさ。SkyAtmosphere.h の定数と一致していること
static const float2 kTransmittanceLutSize = float2(256.0f, 64.0f);
static const float2 kMultiScatterLutSize = float2(32.0f, 32.0f);

/// <summary>その高さの空気の性質</summary>
struct AtmosphereMedium
{
    float3 scattering;     // 散乱(レイリー＋ミー)
    float3 rayleigh;       // そのうちレイリー
    float3 mie;            // そのうちミー
    float3 extinction;     // 散乱＋吸収
};

AtmosphereMedium SampleMedium(float3 position)
{
    float height = max(length(position) - bottomRadius, 0.0f);
    float rayleighDensity = exp(rayleighDensityScale * height);
    float mieDensity = exp(mieDensityScale * height);
    float ozoneDensity = saturate(1.0f - abs(height - ozoneCenter) / ozoneWidth);

    AtmosphereMedium medium;
    medium.rayleigh = rayleighScattering * rayleighDensity;
    medium.mie = mieScattering * mieDensity;
    medium.scattering = medium.rayleigh + medium.mie;
    medium.extinction = medium.rayleigh + mieExtinction * mieDensity + ozoneAbsorption * ozoneDensity;
    return medium;
}

/// <summary>原点から dir へ進んだとき、半径 radius の球に最初に当たる距離。当たらなければ負</summary>
float RaySphere(float3 origin, float3 dir, float radius)
{
    float b = dot(origin, dir);
    float c = dot(origin, origin) - radius * radius;
    float discriminant = b * b - c;
    if (discriminant < 0.0f)
    {
        return -1.0f;
    }
    float s = sqrt(discriminant);
    float near = -b - s;
    float far = -b + s;
    if (near >= 0.0f)
    {
        return near;
    }
    return (far >= 0.0f) ? far : -1.0f;
}

// ---- 透過率の数表 (横: 天頂角, 縦: 高さ) ----

void TransmittanceUvToParams(float2 uv, out float viewHeight, out float viewZenithCos)
{
    float H = sqrt(topRadius * topRadius - bottomRadius * bottomRadius);
    float rho = H * uv.y;
    viewHeight = sqrt(rho * rho + bottomRadius * bottomRadius);

    float dMin = topRadius - viewHeight;
    float dMax = rho + H;
    float d = dMin + uv.x * (dMax - dMin);
    viewZenithCos = (d == 0.0f) ? 1.0f : (H * H - rho * rho - d * d) / (2.0f * viewHeight * d);
    viewZenithCos = clamp(viewZenithCos, -1.0f, 1.0f);
}

float2 TransmittanceParamsToUv(float viewHeight, float viewZenithCos)
{
    float H = sqrt(max(topRadius * topRadius - bottomRadius * bottomRadius, 0.0f));
    float rho = sqrt(max(viewHeight * viewHeight - bottomRadius * bottomRadius, 0.0f));
    float discriminant = viewHeight * viewHeight * (viewZenithCos * viewZenithCos - 1.0f) + topRadius * topRadius;
    float d = max(-viewHeight * viewZenithCos + sqrt(max(discriminant, 0.0f)), 0.0f);
    float dMin = topRadius - viewHeight;
    float dMax = rho + H;
    return float2((d - dMin) / (dMax - dMin), rho / H);
}

/// <summary>数表の端を半texel 内側へ寄せる</summary>
float2 ClampLutUv(float2 uv, float2 size)
{
    float2 halfTexel = 0.5f / size;
    return clamp(uv, halfTexel, 1.0f - halfTexel);
}

/// <summary>その点から太陽へ、地表に遮られずに届く割合</summary>
float3 SunTransmittance(Texture2D<float4> transmittanceLut, SamplerState smp, float3 position, float3 sunDir)
{
    float height = length(position);
    float3 up = position / height;
    float sunZenithCos = dot(sunDir, up);
    // 地平線の境目を少しぼかす
    float horizonCos = -sqrt(max(1.0f - (bottomRadius * bottomRadius) / (height * height), 0.0f));
    float visible = smoothstep(horizonCos - 0.01f, horizonCos + 0.01f, sunZenithCos);
    float2 uv = ClampLutUv(TransmittanceParamsToUv(height, sunZenithCos), kTransmittanceLutSize);
    return transmittanceLut.SampleLevel(smp, uv, 0.0f).rgb * visible;
}

/// <summary>多重散乱の数表から、その点に回り込む光を引く</summary>
/// <remarks>太陽の明るさ1あたりの値</remarks>
float3 MultiScattering(Texture2D<float4> multiScatterLut, SamplerState smp, float3 position, float3 sunDir)
{
    float height = length(position);
    float sunZenithCos = dot(sunDir, position / height);
    float2 uv = float2(sunZenithCos * 0.5f + 0.5f, (height - bottomRadius) / (topRadius - bottomRadius));
    return multiScatterLut.SampleLevel(smp, ClampLutUv(uv, kMultiScatterLutSize), 0.0f).rgb;
}

#endif // ATMOSPHERE_MODEL_HLSLI
