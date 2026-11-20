#ifndef AERIAL_PERSPECTIVE_HLSLI
#define AERIAL_PERSPECTIVE_HLSLI

// 空気遠近の3Dの表 (UE の Aerial Perspective LUT) を引く。作るのは SkyBox/AtmosphereAerialPerspective.CS.hlsl
#include "Fog.hlsli"

// 表の大きさ。SkyAtmosphere.h の定数と一致していること
static const float3 kAerialLutSize = float3(32.0f, 32.0f, 32.0f);

// SkyAtmosphere.h の AerialPerspectiveParams と並びが一致していること
struct AerialPerspectiveParams
{
    float4x4 invViewProj;   // 表を作ったときのカメラ
    float3 sunIlluminance;  // 太陽の色と強さ × 空に掛けている色
    uint enabled;           // 0 ならこのフレームは表を作っていない
    float2 invScreenSize;   // 1 / 描画解像度
    float invMaxDistance;   // 1 / 表の届く距離 (m)。遠クリップと同じ
    float maxDistanceKm;    // 表の届く距離 × 空気遠近の倍率 (km)
};
ConstantBuffer<AerialPerspectiveParams> gAerialPerspective : register(b9);

// rgb = 上に乗る空気の色
Texture3D<float4> gAerialScatter : register(t0, space12);
// rgb = 元の色に掛かる割合
Texture3D<float4> gAerialTransmittance : register(t1, space12);

/// <summary>空気遠近で元の色に掛かる割合と、上に乗る空気の色</summary>
/// <remarks>表を作っていなければ式で求める</remarks>
/// <param name="toPoint">カメラから対象までのベクトル</param>
/// <param name="screenUV">対象が写る画面の位置 (0〜1)</param>
/// <returns>空気遠近が掛かるか</returns>
bool AerialPerspectiveTermsAt(FogParams fog, SamplerState smp, float3 toPoint, float2 screenUV,
                              out float3 transmittance, out float3 inScatter)
{
    if (gAerialPerspective.enabled == 0)
    {
        return AerialPerspectiveTerms(fog, toPoint, transmittance, inScatter);
    }
    transmittance = float3(1.0f, 1.0f, 1.0f);
    inScatter = float3(0.0f, 0.0f, 0.0f);
    if (fog.aerialDistanceScale <= 0.0f)
    {
        return false;
    }

    // 奥行きの段は距離の平方根で並ぶ
    const float3 halfTexel = 0.5f / kAerialLutSize;
    float slice = sqrt(saturate(length(toPoint) * gAerialPerspective.invMaxDistance));
    float3 coord = clamp(float3(screenUV, slice), halfTexel, 1.0f - halfTexel);
    // 最初の段より手前は距離に比例して 0 へ寄せる
    float nearWeight = saturate((slice * slice) / (halfTexel.z * halfTexel.z));

    inScatter = gAerialScatter.SampleLevel(smp, coord, 0.0f).rgb * nearWeight;
    transmittance = lerp(float3(1.0f, 1.0f, 1.0f), gAerialTransmittance.SampleLevel(smp, coord, 0.0f).rgb, nearWeight);
    return true;
}

/// <summary>空気遠近。遠くの物ほど空気の色が乗る</summary>
/// <param name="additive">0以外なら加算合成の物。空気の色は乗せない</param>
float3 ApplyAerialPerspectiveAt(FogParams fog, SamplerState smp, float3 color, float3 toPoint, float2 screenUV, float additive)
{
    float3 transmittance;
    float3 inScatter;
    if (!AerialPerspectiveTermsAt(fog, smp, toPoint, screenUV, transmittance, inScatter))
    {
        return color;
    }
    return color * transmittance + ((additive != 0.0f) ? float3(0.0f, 0.0f, 0.0f) : inScatter);
}

/// <summary>ApplyAerialPerspectiveAt を外す</summary>
float3 RemoveAerialPerspectiveAt(FogParams fog, SamplerState smp, float3 color, float3 toPoint, float2 screenUV)
{
    float3 transmittance;
    float3 inScatter;
    if (!AerialPerspectiveTermsAt(fog, smp, toPoint, screenUV, transmittance, inScatter))
    {
        return color;
    }
    return (color - inScatter) / max(transmittance, 1e-3f);
}

#endif // AERIAL_PERSPECTIVE_HLSLI
