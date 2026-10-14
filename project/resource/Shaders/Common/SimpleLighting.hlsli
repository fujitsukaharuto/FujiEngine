#ifndef SIMPLE_LIGHTING_HLSLI
#define SIMPLE_LIGHTING_HLSLI

// PBR を使わないときの BRDF (Lambert + Blinn-Phong)。明るさの尺度は PBR.hlsli と同じ
#include "PBR.hlsli"

// AllLights::lightingMode の値。C++ 側 (LightManager.h の LightingMode) と同じ値であること
static const uint kLightingModeSimple = 0; // Lambert + Blinn-Phong
static const uint kLightingModePBR = 1;    // Cook-Torrance

// これより鋭いハイライトは1画素より小さくなってちらつく
static const float kMaxShininess = 2048.0f;

/// <summary>1光源ぶんの BRDF。引数と戻り値は PBR.hlsli の BRDF と同じ</summary>
float3 SimpleBRDF(float3 N, float3 V, float3 L, float3 diffuseColor, float3 f0, float roughness)
{
    float NdotL = dot(N, L);
    if (NdotL <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 H = normalize(V + L);
    float NdotH = saturate(dot(N, H));

    // GGX と同じ広がりになる指数
    float a = roughness * roughness;
    float shininess = min(2.0f / max(a * a, 1e-4f) - 2.0f, kMaxShininess);
    float3 specular = f0 * ((shininess + 2.0f) / (8.0f * kPI)) * pow(NdotH, max(shininess, 1e-4f));

    return (diffuseColor / kPI + specular) * NdotL;
}

#endif // SIMPLE_LIGHTING_HLSLI
