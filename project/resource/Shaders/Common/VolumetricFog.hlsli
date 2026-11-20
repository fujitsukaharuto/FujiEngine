#ifndef VOLUMETRIC_FOG_HLSLI
#define VOLUMETRIC_FOG_HLSLI

// ボリュメトリックフォグ (UE の Volumetric Fog / Hillaire 2015)。
// カメラの前を画面 8px 角 × 奥行きの段の箱に分け、霧が受けた光と通り抜ける割合を手前から積んだ3Dの表を引く。
// 作るのは VolumetricFogPass (Raytracing/VolumetricFogScatter.CS → VolumetricFogIntegrate.CS)。
// 表が届くのは maxDistance まで。その先は Fog.hlsli の式の霧が受け持つ
#include "Fog.hlsli"

// VolumetricFogPass.h の VolumetricFogParams と並びが一致していること
struct VolumetricFogParams
{
    float4x4 invViewProj;   // 表を作ったカメラ
    float4x4 prevViewProj;  // 1つ前のフレームのカメラ。履歴を引く位置を求める
    float3 cameraPosition;
    uint enabled;           // 0 ならこのフレームは表を作っていない
    float3 cameraForward;   // カメラの向き (ビュー空間の +Z)
    float maxDistance;      // 最後の段の中心の奥行き (m)。式の霧はここから先
    float3 gridZParams;     // 段 = log2(奥行き × x + y) × z (UE の GridZParams)
    float historyWeight;    // 前のフレームを混ぜる割合。0 なら混ぜない
    uint3 gridSize;
    uint padGrid;
    float2 screenToGridUV;  // 画面の uv を表の uv へ。画面が 8px で割り切れないときだけ1未満
    float2 invScreenSize;   // 1 / 描画解像度
    float3 jitter;          // このフレームに箱の中で値を求める位置 (0〜1)
    float phaseG;           // 散乱の向きの偏り (Henyey-Greenstein の g)
    float3 albedo;          // 散乱 / 消散
    float extinctionScale;  // 霧の濃さに掛ける倍率
    float3 emissive;        // 霧が自分で出す光 (1mあたり)
    float sunScale;         // 平行光源の光の倍率
    float skyLightScale;    // 空からの光の倍率
    float localLightScale;  // 点光源・スポット・影なしのライトの光の倍率
    float2 padScale;
    // 局所の霧 (gLocalFogVolumes) を式で積むときの光。UE の Local Fog Volume と同じく影は無い
    float3 lightDirection;  // 0番の平行光源へ向かう単位ベクトル
    uint localFogVolumeCount; // 0 なら局所の霧は無い。並びは奥から手前
    float3 lightRadiance;   // 0番の平行光源の色 × 強さ
    float padLight;
    float3 ambientSky;      // 半球アンビエントの上の色 × 強さ
    float padSky;
    float3 ambientGround;   // 同、下の色
    float padGround;
};
ConstantBuffer<VolumetricFogParams> gVolumetricFog : register(b10);

// rgb = カメラからその奥行きまでに霧が散らした光、a = その奥行きまで光が通り抜ける割合
Texture3D<float4> gVolumetricFogVolume : register(t0, space14);

// 局所の霧 (UE の Local Fog Volume)。球を拡縮・回転した楕円体。VolumetricFogPass.h の LocalFogVolumeGPU と並びが一致していること
struct LocalFogVolume
{
    float4x4 worldToUnit;    // ワールド → 半径1の球の中 (mul(float4(p, 1), worldToUnit))
    float3 albedo;           // 散乱 / 消散
    float radialExtinction;  // 中心の濃さ (1mあたり)。縁へ (1 - r^2) で薄まって縁で 0
    float3 emissive;         // 霧が見せる自分の光。濃いほど強く見える (UE と同じく (1 - 透過率) が掛かる)
    float heightExtinction;  // 高さの霧の濃さ (1mあたり)。基準の高さでこの値。楕円体の中だけ
    float heightFalloff;     // 高さの霧の薄まり方 (1mあたり)
    float baseHeight;        // 高さの霧の基準の高さ (ワールド)
    float phaseG;            // 式で積むときの散乱の向きの偏り
    float padVolume;
};
StructuredBuffer<LocalFogVolume> gLocalFogVolumes : register(t1, space14);

/// <summary>奥行き (ビュー空間の z) から段の番号。段の中心は番号 + 0.5</summary>
float VolumetricFogSliceFromDepth(float viewDepth)
{
    float3 p = gVolumetricFog.gridZParams;
    return log2(max(viewDepth * p.x + p.y, 1e-6f)) * p.z;
}

/// <summary>段の番号から奥行き (ビュー空間の z)</summary>
float VolumetricFogDepthFromSlice(float slice)
{
    float3 p = gVolumetricFog.gridZParams;
    return (exp2(slice / p.z) - p.y) / p.x;
}

/// <summary>カメラから対象までの奥行き (ビュー空間の z)</summary>
float VolumetricFogViewDepth(float3 toPoint)
{
    return dot(toPoint, gVolumetricFog.cameraForward);
}

/// <summary>式の霧を数え始める距離。表の届く奥行きまでは表が受け持つ</summary>
/// <remarks>表を作っていなければ 0。UE の CalculateHeightFog の ExcludeDistance と同じ</remarks>
/// <param name="toPoint">カメラから対象へのベクトル</param>
float VolumetricFogExcludeDistance(float3 toPoint)
{
    if (gVolumetricFog.enabled == 0)
    {
        return 0.0f;
    }
    float distance = length(toPoint);
    float cosAngle = dot(toPoint, gVolumetricFog.cameraForward) / max(distance, 1e-6f);
    return (cosAngle > 1e-4f) ? gVolumetricFog.maxDistance / cosAngle : 0.0f;
}

/// <summary>その奥行きまでの霧の光と、通り抜ける割合</summary>
/// <param name="screenUV">対象が写る画面の位置 (0〜1)</param>
/// <param name="viewDepth">対象の奥行き。空は大きな値でよい (最後の段の値になる)</param>
/// <returns>表を引いたか</returns>
bool VolumetricFogTermsAt(SamplerState smp, float2 screenUV, float viewDepth, out float transmittance, out float3 inScatter)
{
    transmittance = 1.0f;
    inScatter = float3(0.0f, 0.0f, 0.0f);
    if (gVolumetricFog.enabled == 0)
    {
        return false;
    }

    // 既定のサンプラーは WRAP なので、端の向こうを混ぜないよう半texel 内側へ寄せる
    float3 size = float3(gVolumetricFog.gridSize);
    float3 halfTexel = 0.5f / size;
    float3 uvw = float3(screenUV * gVolumetricFog.screenToGridUV, VolumetricFogSliceFromDepth(viewDepth) / size.z);
    float4 value = gVolumetricFogVolume.SampleLevel(smp, clamp(uvw, halfTexel, 1.0f - halfTexel), 0.0f);

    // 最初の段の中心より手前は、表の始まり (霧の始まり) から中心までで 0 から伸ばす
    float nearStart = VolumetricFogDepthFromSlice(0.0f);
    float nearCenter = VolumetricFogDepthFromSlice(0.5f);
    float nearWeight = saturate((viewDepth - nearStart) / max(nearCenter - nearStart, 1e-4f));
    inScatter = value.rgb * nearWeight;
    transmittance = lerp(1.0f, value.a, nearWeight);
    return true;
}

/// <summary>霧を掛ける。式の霧を掛けた後に呼ぶ (表の霧の方がカメラに近い)</summary>
/// <param name="additive">0以外なら加算合成の物。光は足さず暗くするだけ</param>
float3 ApplyVolumetricFogAt(SamplerState smp, float3 color, float2 screenUV, float viewDepth, float additive)
{
    float transmittance;
    float3 inScatter;
    if (!VolumetricFogTermsAt(smp, screenUV, viewDepth, transmittance, inScatter))
    {
        return color;
    }
    return color * transmittance + ((additive != 0.0f) ? float3(0.0f, 0.0f, 0.0f) : inScatter);
}

/// <summary>ApplyVolumetricFogAt を外す</summary>
float3 RemoveVolumetricFogAt(SamplerState smp, float3 color, float2 screenUV, float viewDepth)
{
    float transmittance;
    float3 inScatter;
    if (!VolumetricFogTermsAt(smp, screenUV, viewDepth, transmittance, inScatter))
    {
        return color;
    }
    return (color - inScatter) / max(transmittance, 1e-3f);
}

/// <summary>その奥行きまで光が通り抜ける割合だけ</summary>
float VolumetricFogTransmittanceAt(SamplerState smp, float2 screenUV, float viewDepth)
{
    float transmittance;
    float3 inScatter;
    VolumetricFogTermsAt(smp, screenUV, viewDepth, transmittance, inScatter);
    return transmittance;
}

/// <summary>Henyey-Greenstein の位相関数</summary>
/// <param name="cosTheta">光の進む向きと、散ってカメラへ向かう向きのなす角の cos</param>
float VolumetricFogPhaseHG(float cosTheta, float g)
{
    float denominator = max(1.0f + g * g - 2.0f * g * cosTheta, 1e-4f);
    return (1.0f - g * g) / (4.0f * 3.14159265359f * denominator * sqrt(denominator));
}

/// <summary>その点の局所の霧。ボリュメトリックフォグの箱へ流し込むときに使う</summary>
/// <param name="scattering">散乱 (アルベド × 濃さ) の合計</param>
/// <param name="emissive">霧が出す光 (1mあたり) の合計</param>
/// <returns>濃さ (消散) の合計</returns>
float LocalFogVolumesAt(float3 position, out float3 scattering, out float3 emissive)
{
    float extinction = 0.0f;
    scattering = float3(0.0f, 0.0f, 0.0f);
    emissive = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gVolumetricFog.localFogVolumeCount; i++)
    {
        LocalFogVolume volume = gLocalFogVolumes[i];
        float3 u = mul(float4(position, 1.0f), volume.worldToUnit).xyz;
        float r2 = dot(u, u);
        if (r2 >= 1.0f)
        {
            continue;
        }
        float density = volume.radialExtinction * (1.0f - r2) +
                        volume.heightExtinction * exp(clamp(-volume.heightFalloff * (position.y - volume.baseHeight), -20.0f, 20.0f));
        extinction += density;
        scattering += volume.albedo * density;
        emissive += volume.emissive * density;
    }
    return extinction;
}

/// <summary>局所の霧1つを、視線の [tMin, tMax] の区間で式で積む (UE の Local Fog Volume の解析積分)</summary>
/// <param name="direction">視線の向き (単位ベクトル)</param>
/// <returns>区間が楕円体を通るか</returns>
bool LocalFogVolumeTerms(LocalFogVolume volume, float3 origin, float3 direction, float tMin, float tMax,
                         out float transmittance, out float3 inScatter)
{
    transmittance = 1.0f;
    inScatter = float3(0.0f, 0.0f, 0.0f);
    float3 uo = mul(float4(origin, 1.0f), volume.worldToUnit).xyz;
    float3 ud = mul(float4(direction, 0.0f), volume.worldToUnit).xyz;
    // |uo + t ud|^2 = 1 の2つの解が、楕円体に入る距離と出る距離
    float a = dot(ud, ud);
    float b = dot(uo, ud);
    float c = dot(uo, uo) - 1.0f;
    float discriminant = b * b - a * c;
    if (a <= 1e-12f || discriminant <= 0.0f)
    {
        return false;
    }
    float root = sqrt(discriminant);
    float t0 = max((-b - root) / a, tMin);
    float t1 = min((-b + root) / a, tMax);
    if (t1 <= t0)
    {
        return false;
    }

    // 入った点から測る。遠くから見ても桁落ちしない
    float travel = t1 - t0;
    float3 u0 = uo + ud * t0;
    float c0 = dot(u0, u0);
    float b0 = dot(u0, ud);
    // ∫ (1 - |u0 + s ud|^2) ds (s = 0〜travel)。|u0 + s ud|^2 = c0 + 2 b0 s + a s^2
    float radial = travel * (1.0f - c0) - b0 * travel * travel - a * travel * travel * travel / 3.0f;
    float opticalDepth = volume.radialExtinction * max(radial, 0.0f) +
                         FogLayerOpticalDepth(volume.heightExtinction, volume.heightFalloff, volume.baseHeight,
                                              origin.y + direction.y * t0, direction.y, travel);
    transmittance = exp(-opticalDepth);

    // 光は区間の中で一様とみなす。散乱 = アルベド × 消散 なので、積むと (1 - 透過率) が掛かる
    float3 sun = gVolumetricFog.lightRadiance * VolumetricFogPhaseHG(dot(direction, gVolumetricFog.lightDirection), volume.phaseG);
    // 空からの光はカメラへ向いた面に届く光で代える (向き -direction の半球アンビエント)
    float3 sky = lerp(gVolumetricFog.ambientGround, gVolumetricFog.ambientSky, -direction.y * 0.5f + 0.5f);
    inScatter = (volume.albedo * (sun + sky) + volume.emissive) * (1.0f - transmittance);
    return true;
}

/// <summary>局所の霧を式で掛ける。式の霧の後、表の霧の前に呼ぶ</summary>
/// <param name="direction">視線の向き (単位ベクトル)</param>
/// <param name="distance">カメラから対象までの距離。空は大きな値</param>
/// <param name="excludeDistance">ここまではボリュメトリックフォグの表が受け持つ (VolumetricFogExcludeDistance)</param>
/// <param name="additive">0以外なら加算合成の物。光は足さず暗くするだけ</param>
float3 ApplyLocalFogVolumes(float3 color, float3 origin, float3 direction, float distance, float excludeDistance, float additive)
{
    // 並びは奥から手前
    for (uint i = 0; i < gVolumetricFog.localFogVolumeCount; i++)
    {
        float transmittance;
        float3 inScatter;
        if (LocalFogVolumeTerms(gLocalFogVolumes[i], origin, direction, excludeDistance, distance, transmittance, inScatter))
        {
            color = color * transmittance + ((additive != 0.0f) ? float3(0.0f, 0.0f, 0.0f) : inScatter);
        }
    }
    return color;
}

/// <summary>ApplyLocalFogVolumes を外す</summary>
float3 RemoveLocalFogVolumes(float3 color, float3 origin, float3 direction, float distance, float excludeDistance)
{
    for (int i = int(gVolumetricFog.localFogVolumeCount) - 1; i >= 0; i--)
    {
        float transmittance;
        float3 inScatter;
        if (LocalFogVolumeTerms(gLocalFogVolumes[i], origin, direction, excludeDistance, distance, transmittance, inScatter))
        {
            color = (color - inScatter) / max(transmittance, 1e-3f);
        }
    }
    return color;
}

/// <summary>局所の霧を合わせて、元の色がどれだけ残るか</summary>
float LocalFogVolumesTransmittance(float3 origin, float3 direction, float distance, float excludeDistance)
{
    float total = 1.0f;
    for (uint i = 0; i < gVolumetricFog.localFogVolumeCount; i++)
    {
        float transmittance;
        float3 inScatter;
        if (LocalFogVolumeTerms(gLocalFogVolumes[i], origin, direction, excludeDistance, distance, transmittance, inScatter))
        {
            total *= transmittance;
        }
    }
    return total;
}

#endif // VOLUMETRIC_FOG_HLSLI
