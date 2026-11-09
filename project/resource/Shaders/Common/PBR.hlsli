#ifndef PBR_HLSLI
#define PBR_HLSLI

// 物理ベースの BRDF (Cook-Torrance / metallic-roughness ワークフロー)。
// 減衰(距離やスポットの絞り)は呼び出し側の責任で、ここには入れない

static const float kPI = 3.14159265359f;

// 誘電体(非金属)の垂直入射反射率。おおよそ4%で、業界の慣用値
static const float kDielectricF0 = 0.04f;

/// <summary>GGX(Trowbridge-Reitz) の法線分布。ハイライトの広がりを決める</summary>
float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / max(kPI * d * d, 1e-7f);
}

/// <summary>Smith の幾何減衰(Schlick-GGX 近似、直接光用の k)</summary>
float G_SmithSchlick(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float gv = NdotV / (NdotV * (1.0f - k) + k);
    float gl = NdotL / (NdotL * (1.0f - k) + k);
    return gv * gl;
}

/// <summary>Schlick のフレネル近似</summary>
float3 F_Schlick(float3 f0, float VdotH)
{
    float f = pow(saturate(1.0f - VdotH), 5.0f);
    return f0 + (1.0f - f0) * f;
}

/// <summary>
/// 1光源ぶんの BRDF。戻り値に光源の放射輝度を掛けると最終的な寄与になる
/// </summary>
/// <param name="N">法線(正規化済み)</param>
/// <param name="V">視線方向(面 → カメラ、正規化済み)</param>
/// <param name="L">光源方向(面 → 光源、正規化済み)</param>
/// <param name="diffuseColor">拡散反射色。金属分を落としたアルベド</param>
/// <param name="f0">垂直入射時の鏡面反射率</param>
/// <param name="roughness">粗さ</param>
/// <param name="energyCompensation">鏡面に掛ける多重散乱の倍率 (1 で掛けない)</param>
float3 BRDF(float3 N, float3 V, float3 L, float3 diffuseColor, float3 f0, float roughness, float3 energyCompensation)
{
    float NdotL = dot(N, L);
    if (NdotL <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 H = normalize(V + L);
    // 0除算を避けるため下限を置く。真横から見た面でハイライトが発散するのを防ぐ
    float NdotV = max(dot(N, V), 1e-4f);
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    float D = D_GGX(NdotH, roughness);
    float G = G_SmithSchlick(NdotV, NdotL, roughness);
    float3 F = F_Schlick(f0, VdotH);

    float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-7f) * energyCompensation;

    // 鏡面へ回った分だけ拡散を減らす(エネルギー保存)。
    // 金属分は diffuseColor 側で既に落としてあるのでここでは掛けない
    float3 kd = 1.0f - F;

    return (kd * diffuseColor / kPI + specular) * NdotL;
}

/// <summary>画素の中の法線のばらつきぶん粗さを上げる (Tokuyoshi & Kaplanyan 2019)</summary>
/// <param name="normal">法線マップを当てた後の法線</param>
/// <param name="roughness">見た目の粗さ (2乗が GGX の a)</param>
float SpecularAntiAliasing(float3 normal, float roughness)
{
    // 画素の幅のフィルタの分散と、広げる量の上限
    const float kVariance = 0.15f;
    const float kThreshold = 0.2f;
    float3 du = ddx(normal);
    float3 dv = ddy(normal);
    float variance = kVariance * (dot(du, du) + dot(dv, dv));
    float kernelRoughness2 = min(2.0f * variance, kThreshold);
    float a = roughness * roughness;
    float filtered = saturate(a * a + kernelRoughness2);
    return sqrt(sqrt(filtered));
}

/// <summary>
/// 半球アンビエント。法線の上下で空色と地面色を混ぜるだけの簡易版
/// </summary>
/// <remarks>IBL 経路では Common/IBL.hlsli の GetAmbientDiffuse が焼いた放射照度に差し替える</remarks>
float3 HemisphereAmbient(float3 N, float3 skyColor, float3 groundColor, float intensity)
{
    float t = N.y * 0.5f + 0.5f;
    return lerp(groundColor, skyColor, t) * intensity;
}

#endif // PBR_HLSLI
