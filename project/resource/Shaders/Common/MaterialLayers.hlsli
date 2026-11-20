#ifndef MATERIAL_LAYERS_HLSLI
#define MATERIAL_LAYERS_HLSLI

// 標準の面に足す層 (Filament の standard モデル / glTF の KHR_materials_clearcoat・sheen・anisotropy)。
// 下から 異方性のある面 → 布の光沢 (sheen) → クリアコート の順に重ね、上の層が下の層を暗くする
#include "PBR.hlsli"

struct MaterialLayers
{
    bool hasLayers;             // 布の光沢かクリアコートがある
    float clearCoat;            // クリアコートの強さ (0〜1)
    float clearCoatRoughness;   // 膜の見た目の粗さ
    float3 clearCoatNormal;     // 膜の法線。法線マップを掛ける前の面 (Filament の既定と同じ)
    float anisotropy;           // -1〜1。正なら接線の向きへ粗さを伸ばす。0 で等方
    float3 anisotropyT;         // 接線
    float3 anisotropyB;         // 従接線
    float3 sheenColor;          // 布の光沢の色。黒なら無し
    float sheenRoughness;       // 布の光沢の見た目の粗さ
    float sheenAlbedo;          // 布の光沢の反射率 E(N・V)
    float sheenScaling;         // 下の層に掛ける 1 - max(sheenColor) × E(N・V)
};

/// <summary>クリアコートの下の面の f0。膜 (IOR 1.5) と接すると反射率が下がる (Filament の f0ClearCoatToSurface)</summary>
float3 F0UnderClearCoat(float3 f0)
{
    return saturate(f0 * (f0 * (0.941892f - 0.263008f * f0) + 0.346479f) - 0.0285998f);
}

/// <summary>異方性のある面の BRDF × N・L。拡散は等方の BRDF と同じ</summary>
/// <remarks>D は異方性 GGX (Burley 2012)、V は高さ相関 Smith の異方性版 (Heitz 2014)。式は Filament と同じ</remarks>
float3 BRDFAnisotropic(float3 N, float3 V, float3 L, float3 diffuseColor, float3 f0, float roughness, float3 energyCompensation,
                       float3 T, float3 B, float anisotropy)
{
    float NdotL = dot(N, L);
    if (NdotL <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    float3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 1e-4f);
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    float a = roughness * roughness;
    float at = max(a * (1.0f + anisotropy), 1e-3f);
    float ab = max(a * (1.0f - anisotropy), 1e-3f);

    float a2 = at * ab;
    float3 d = float3(ab * dot(T, H), at * dot(B, H), a2 * NdotH);
    float b2 = a2 / max(dot(d, d), 1e-12f);
    float D = a2 * b2 * b2 / kPI;

    float lambdaV = NdotL * length(float3(at * dot(T, V), ab * dot(B, V), NdotV));
    float lambdaL = NdotV * length(float3(at * dot(T, L), ab * dot(B, L), NdotL));
    float visibility = saturate(0.5f / max(lambdaV + lambdaL, 1e-7f));

    float3 F = F_Schlick(f0, VdotH);
    float3 specular = D * visibility * F * energyCompensation;
    return ((1.0f - F) * diffuseColor / kPI + specular) * NdotL;
}

/// <summary>直接光に布の光沢とクリアコートを重ねる</summary>
/// <param name="base">下の面の BRDF × N・L</param>
float3 LayeredDirect(float3 base, float3 N, float3 V, float3 L, MaterialLayers layers)
{
    float3 result = base;
    float3 H = normalize(V + L);
    if (any(layers.sheenColor > 0.0f))
    {
        float NdotL = saturate(dot(N, L));
        float NdotV = max(dot(N, V), 1e-4f);
        float sheen = D_Charlie(saturate(dot(N, H)), layers.sheenRoughness) * V_Neubelt(NdotV, NdotL) * NdotL;
        result = result * layers.sheenScaling + layers.sheenColor * sheen;
    }
    if (layers.clearCoat > 0.0f)
    {
        float LdotH = saturate(dot(L, H));
        float coatNdotL = saturate(dot(layers.clearCoatNormal, L));
        float coatFresnel = F_Schlick(float3(kDielectricF0, kDielectricF0, kDielectricF0), LdotH).x * layers.clearCoat;
        float coat = D_GGX(saturate(dot(layers.clearCoatNormal, H)), layers.clearCoatRoughness) * V_Kelemen(LdotH) * coatFresnel;
        result = result * (1.0f - coatFresnel) + coat * coatNdotL;
    }
    return result;
}

/// <summary>異方性の面で映り込みを引く向きの法線 (Filament の bent normal)</summary>
/// <param name="roughness">面の見た目の粗さ</param>
float3 AnisotropicReflectionNormal(float3 N, float3 V, float roughness, MaterialLayers layers)
{
    float3 direction = (layers.anisotropy >= 0.0f) ? layers.anisotropyB : layers.anisotropyT;
    float3 tangent = cross(direction, V);
    float3 bentNormal = cross(tangent, direction);
    // 視線が伸ばす向きと平行なら曲げようがない
    if (dot(bentNormal, bentNormal) < 1e-8f)
    {
        return N;
    }
    float bend = abs(layers.anisotropy) * saturate(5.0f * roughness);
    return normalize(lerp(N, bentNormal, bend));
}

#endif // MATERIAL_LAYERS_HLSLI
