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

/// <summary>鏡面の反射率。split-sum の第2項を数表から引いて F0 に当てる</summary>
/// <remarks>フレネルと幾何減衰を粗さと N・V の2変数に押し込んだもの</remarks>
float3 EnvironmentBRDF(float3 f0, float NdotV, float roughness, SamplerState smp)
{
    // 既定のサンプラーが WRAP なので、端をそのまま引くと双一次補間が反対側の値を混ぜる。
    // 半texel 内側へ寄せて防ぐ
    const float halfTexel = 0.5f / kBRDFLutSize;
    float2 uv = clamp(float2(NdotV, roughness), halfTexel, 1.0f - halfTexel);

    float2 ab = gBRDFLut.SampleLevel(smp, uv, 0.0f);
    return f0 * ab.x + ab.y;
}

/// <summary>拡散のアンビエント。掛ける前の「回り込んでくる光の色」を返す</summary>
/// <param name="ambientMode">AllLights::ambientMode</param>
float3 GetAmbientDiffuse(float3 N, uint ambientMode, float3 skyColor, float3 groundColor,
                         float intensity, SamplerState smp)
{
    if (ambientMode == kAmbientModeIBL)
    {
        // 焼く側が cosine 重みの平均で持っているので、一様な空なら空の色がそのまま返る。
        // Hemisphere 経路と同じ尺度なので intensity をそのまま掛けてよい
        return gIrradianceMap.SampleLevel(smp, N, 0.0f).rgb * intensity;
    }

    return HemisphereAmbient(N, skyColor, groundColor, intensity);
}

/// <summary>鏡面のアンビエント(映り込み)。フレネルまで済ませた寄与を返す</summary>
/// <remarks>IBL 経路は粗さでミップを選ぶので粗い面ほどぼやける</remarks>
/// <param name="V">視線方向(面 → カメラ、正規化済み)</param>
/// <param name="occludedColor">遮蔽された向きに映す色</param>
/// <param name="enableReflection">0なら遮蔽を見ない</param>
/// <param name="ambientMode">AllLights::ambientMode</param>
float3 GetAmbientSpecular(float3 worldPos, float3 N, float3 V, float roughness, float3 f0,
                          float3 occludedColor, float maxDistance, uint enableReflection,
                          uint ambientMode, SamplerState smp)
{
    float NdotV = saturate(dot(N, V));

    if (ambientMode == kAmbientModeIBL)
    {
        float3 R = reflect(-V, N);

        // 粗さがそのままミップ番号になるように焼いてある
        float3 prefiltered = gPrefilteredEnv.SampleLevel(smp, R, roughness * kPrefilteredMaxMip).rgb;
        if (enableReflection != 0 && maxDistance > 0.0f &&
            TraceShadowRay(worldPos, R, N, maxDistance) == 0.0f)
        {
            prefiltered = occludedColor;
        }

        return prefiltered * EnvironmentBRDF(f0, NdotV, roughness, smp);
    }

    float3 environmentColor = TraceReflection(worldPos, N, V, occludedColor,
                                              maxDistance, enableReflection, smp);
    // 粗い面ほど映り込みがぼやけて弱く見えるので、その代用として単純に落とす
    return environmentColor * F_Schlick(f0, NdotV) * (1.0f - roughness);
}

#endif // IBL_HLSLI
