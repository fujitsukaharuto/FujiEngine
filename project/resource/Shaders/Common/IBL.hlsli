#ifndef IBL_HLSLI
#define IBL_HLSLI

// 環境マップから焼いた IBL を引くヘッダ。経路(kAmbientMode*)の切り替えもここに閉じ込める。
// 焼く側は Engine/Graphics/IBL/IBLBaker と Shaders/IBL/*.CS.hlsl
#include "PBR.hlsli"
#include "RayTracedReflection.hlsli"

// space0=gTextures[] / space1=環境マップ系 / space2=TLAS / space3=画面空間の計算済みバッファ。
// gEnvironment(t0) は RayTracedReflection.hlsli が宣言しているので、その続きに並べる
TextureCube<float4> gIrradianceMap : register(t1, space1);
TextureCube<float4> gPrefilteredEnv : register(t2, space1);
Texture2D<float2> gBRDFLut : register(t3, space1);

// AllLights::ambientMode の値。C++ 側 (LightManager.h の AmbientMode) と同じ値であること
static const uint kAmbientModeHemisphere = 0; // 空色と地面色の2色補間
static const uint kAmbientModeIBL = 1;        // 焼いたキューブマップを引く

// prefiltered の最大ミップ番号。C++ 側 (IBLBaker::kPrefilteredMipLevels_ - 1) と一致していること
static const float kPrefilteredMaxMip = 4.0f;

// BRDF LUT の1辺。C++ 側 (IBLBaker::kBRDFLutSize_) と一致していること
static const float kBRDFLutSize = 256.0f;

// AllLights::renderFlags のビット。C++ 側 (LightManager.h の RenderFlag) と同じ値であること
static const uint kRenderFlagEnergyCompensation = 1u << 0; // 多重散乱のぶんを足す
static const uint kRenderFlagSpecularOcclusion = 1u << 1;  // AO と法線マップの地平線で映り込みを遮る
static const uint kRenderFlagReflectAll = 1u << 2;         // 全マテリアルに映り込みを付ける
static const uint kRenderFlagGI = 1u << 3;                 // プローブの間接光
static const uint kRenderFlagScreenGI = 1u << 4;           // 画面のプローブの間接光
static const uint kRenderFlagScreenReflection = 1u << 5;   // 映り込みを別パスで求める
static const uint kRenderFlagReflectionBounce = 1u << 6;   // 別パスの映り込みで2回目の跳ね返りも出す
static const uint kRenderFlagSpecularAA = 1u << 7;         // ハイライトのちらつきを粗さで抑える
static const uint kRenderFlagReflectionSceneColor = 1u << 8; // 別パスの映り込みで画面の色を使う (SSR)
static const uint kRenderFlagTranslucentShadow = 1u << 9;    // 半透明の物の色つきの影。TLAS に半透明の物があるときだけ立つ
static const uint kRenderFlagLocalLightAO = 1u << 10;        // 影なしのライトにも AO を掛ける

/// <summary>split-sum の第2項の数表。x が F0 に掛かる係数、y が足す項</summary>
float2 SampleBRDFLut(float NdotV, float roughness, SamplerState smp)
{
    // 既定のサンプラーが WRAP なので、端をそのまま引くと双一次補間が反対側の値を混ぜる。
    // 半texel 内側へ寄せて防ぐ
    const float halfTexel = 0.5f / kBRDFLutSize;
    float2 uv = clamp(float2(NdotV, roughness), halfTexel, 1.0f - halfTexel);
    return gBRDFLut.SampleLevel(smp, uv, 0.0f);
}

/// <summary>鏡面の反射率。split-sum の第2項を数表から引いて F0 に当てる</summary>
/// <remarks>フレネルと幾何減衰を粗さと N・V の2変数にしたもの</remarks>
/// <param name="multiScatter">多重散乱のぶんを足す (Fdez-Aguera 2019)</param>
float3 EnvironmentBRDF(float3 f0, float NdotV, float roughness, SamplerState smp, bool multiScatter)
{
    float2 ab = SampleBRDFLut(NdotV, roughness, smp);
    float3 singleScatter = f0 * ab.x + ab.y;
    if (!multiScatter)
    {
        return singleScatter;
    }

    // 1回で返らなかった残りを、平均のフレネルで跳ね返り続けるとして足す
    float ess = ab.x + ab.y;
    float3 fresnelAverage = f0 + (1.0f - f0) / 21.0f;
    float3 multiple = singleScatter * fresnelAverage * (1.0f - ess) / (1.0f - (1.0f - ess) * fresnelAverage);
    return singleScatter + multiple;
}

/// <summary>直接光の鏡面に掛ける、多重散乱のぶんの倍率 (Filament と同じ近似)</summary>
float3 EnergyCompensation(float3 f0, float NdotV, float roughness, SamplerState smp)
{
    float2 ab = SampleBRDFLut(NdotV, roughness, smp);
    return 1.0f + f0 * (1.0f / max(ab.x + ab.y, 1e-3f) - 1.0f);
}

/// <summary>AO から鏡面の遮蔽を見積もる (Lagarde 2014)</summary>
float SpecularOcclusion(float NdotV, float ao, float roughness)
{
    return saturate(pow(abs(NdotV + ao), exp2(-16.0f * roughness - 1.0f)) - 1.0f + ao);
}

/// <summary>法線マップで反射の向きが面の下へ潜ったぶんを暗くする (Jimenez 2016)</summary>
/// <param name="R">反射の向き</param>
/// <param name="geometricNormal">法線マップを掛ける前の法線</param>
float HorizonOcclusion(float3 R, float3 geometricNormal)
{
    float horizon = saturate(1.0f + dot(R, geometricNormal));
    return horizon * horizon;
}

/// <summary>拡散のアンビエント。掛ける前の「回り込んでくる光の色」を返す</summary>
/// <param name="ambientMode">AllLights::ambientMode</param>
/// <param name="iblScale">IBL 経路のときだけ掛ける倍率</param>
/// <param name="environmentTint">空に掛けている色。IBL 経路のときだけ掛ける</param>
float3 GetAmbientDiffuse(float3 N, uint ambientMode, float3 skyColor, float3 groundColor,
                         float intensity, float iblScale, float3 environmentTint, SamplerState smp)
{
    if (ambientMode == kAmbientModeIBL)
    {
        // 焼く側が cosine 重みの平均で持っているので、一様な空なら空の色がそのまま返る。
        // Hemisphere 経路と同じ尺度なので intensity をそのまま掛けてよい
        return gIrradianceMap.SampleLevel(smp, N, 0.0f).rgb * environmentTint * (intensity * iblScale);
    }

    return HemisphereAmbient(N, skyColor, groundColor, intensity);
}

/// <summary>鏡面のアンビエント(映り込み)。フレネルまで済ませた寄与を返す</summary>
/// <remarks>IBL 経路は粗さでミップを選ぶので粗い面ほどぼやける</remarks>
/// <remarks>レイは飛ばさない。遮られているかは呼び出し側が TraceReflectionHit で調べて渡す</remarks>
/// <param name="V">視線方向(面 → カメラ、正規化済み)</param>
/// <param name="isOccluded">反射方向が物に遮られているか</param>
/// <param name="occludedColor">遮られているときに空の代わりに映す色</param>
/// <param name="ambientMode">AllLights::ambientMode</param>
/// <param name="environmentTint">空に掛けている色</param>
/// <param name="hasEnvironment">空を出しているか。出していなければ skyFallback を映す</param>
/// <param name="skyFallback">空の代わりに映す色</param>
/// <param name="multiScatter">多重散乱のぶんを足すか(IBL 経路だけ)</param>
float3 GetAmbientSpecular(float3 N, float3 V, float roughness, float3 f0,
                          float occludedWeight, float3 occludedColor,
                          uint ambientMode, float3 environmentTint, bool hasEnvironment, float3 skyFallback,
                          bool multiScatter, SamplerState smp)
{
    float NdotV = saturate(dot(N, V));
    float3 R = reflect(-V, N);

    if (ambientMode == kAmbientModeIBL)
    {
        // 粗さがそのままミップ番号になるように焼いてある
        float3 prefiltered = skyFallback;
        if (hasEnvironment)
        {
            prefiltered = gPrefilteredEnv.SampleLevel(smp, R, roughness * kPrefilteredMaxMip).rgb * environmentTint;
        }
        prefiltered = lerp(prefiltered, occludedColor, occludedWeight);

        return prefiltered * EnvironmentBRDF(f0, NdotV, roughness, smp, multiScatter);
    }

    float3 environmentColor = skyFallback;
    if (hasEnvironment)
    {
        environmentColor = gEnvironment.Sample(smp, R).rgb * environmentTint;
    }
    environmentColor = lerp(environmentColor, occludedColor, occludedWeight);
    // 粗い面ほど映り込みがぼやけて弱く見えるので、その代用として単純に落とす
    return environmentColor * F_Schlick(f0, NdotV) * (1.0f - roughness);
}

#endif // IBL_HLSLI
