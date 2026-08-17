#ifndef SCREEN_SPACE_SHADOW_HLSLI
#define SCREEN_SPACE_SHADOW_HLSLI

// 前方描画から平行光源の影を引くためのヘッダ。経路(kShadowMode*)の切り替えもここに閉じ込める。
#include "RayTracedShadow.hlsli"

// 別パスで計算しデノイズ済みの遮蔽率。
// space0=gTextures[] / space1=gEnvironment / space2=TLAS / space3=画面空間の計算済みバッファ
Texture2D<float> gShadowTexture : register(t1, space3);

/// <summary>この画素の遮蔽率を引く。1で光が当たっている、0で完全に影</summary>
/// <remarks>
/// 影バッファは画面と同じ解像度でオフスクリーンRTと画素が一対一に対応しているので、
/// SV_POSITION をそのまま整数化して引ける(補間もサンプラも要らない)
/// </remarks>
float SampleScreenShadow(float4 svPosition)
{
    return gShadowTexture.Load(int3(int2(svPosition.xy), 0));
}

/// <summary>設定された経路で平行光源の影を求める</summary>
/// <remarks>
/// ★画面空間のパスが持っているのは0番の平行光源ぶんだけ。遮蔽率は光源ごとに別の値なので、
/// 2本目以降まで柔らかくするにはバッファを本数ぶん持つ必要がある。
/// 実運用が1本なので、2本目以降はこれまでどおりPSでレイを1本飛ばす形にしてある
/// </remarks>
/// <param name="svPosition">ピクセルシェーダの SV_POSITION</param>
/// <param name="lightIndex">何番目の平行光源か</param>
/// <param name="shadowMask">AllLights::rayTracedShadowMask</param>
/// <param name="shadowMode">AllLights::shadowMode</param>
float GetDirectionalShadow(float4 svPosition, float3 worldPos, float3 L, float3 N,
                           uint lightIndex, uint shadowMask, uint shadowMode)
{
    if (shadowMode == kShadowModeSoft && lightIndex == 0 && (shadowMask & kShadowMaskDirectional) != 0)
    {
        return SampleScreenShadow(svPosition);
    }

    return TraceShadowDirectional(worldPos, L, N, shadowMask);
}

#endif // SCREEN_SPACE_SHADOW_HLSLI
