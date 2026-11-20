#ifndef SCREEN_SPACE_SHADOW_HLSLI
#define SCREEN_SPACE_SHADOW_HLSLI

// 前方描画から平行光源の影を引くためのヘッダ。経路(kShadowMode*)の切り替えもここに閉じ込める。
#include "RayTracedShadow.hlsli"

// 別パスで計算しデノイズ済みの遮蔽率。
// space0=gTextures[] / space1=gEnvironment / space2=TLAS / space3=画面空間の計算済みバッファ
Texture2D<float> gShadowTexture : register(t1, space3);
// 点光源とスポットの柔らかい影。rgb が枠 0〜2 (PunctualShadowPass)
Texture2D<float4> gPunctualShadowTexture : register(t6, space3);

// 柔らかい影の枠。LightManager.h の SoftShadowSlot と同じ値であること
static const uint kSoftShadowSlotCount = 3;
static const uint kSoftShadowSlotSpot = 0x80;

/// <summary>この画素の遮蔽率を引く。1で光が当たっている、0で完全に影</summary>
/// <remarks>影バッファは画面と同じ解像度で画素が一対一に対応するので、SV_POSITION を整数化して引ける</remarks>
float SampleScreenShadow(float4 svPosition)
{
    return gShadowTexture.Load(int3(int2(svPosition.xy), 0));
}

/// <summary>設定された経路で平行光源の影を求める</summary>
/// <remarks>画面空間のパスが持つのは0番の平行光源ぶんだけ。2本目以降はPSでレイを1本飛ばす</remarks>
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

/// <summary>その光源が柔らかい影の何番目の枠か。入っていなければ kSoftShadowSlotCount</summary>
/// <param name="softShadowLights">AllLights::softShadowLights</param>
uint FindSoftShadowSlot(uint softShadowLights, bool isSpot, uint lightIndex)
{
    uint key = lightIndex | (isSpot ? kSoftShadowSlotSpot : 0u);
    for (uint slot = 0; slot < kSoftShadowSlotCount; slot++)
    {
        if (((softShadowLights >> (slot * 8u)) & 0xFFu) == key)
        {
            return slot;
        }
    }
    return kSoftShadowSlotCount;
}

/// <summary>点光源・スポットの影。柔らかい影の枠に入っていれば別パスの結果を引く</summary>
/// <param name="useScreen">別パスの結果を使ってよい画素か (プリパスに載っている不透明な物)</param>
/// <param name="typeBit">kShadowMaskPoint か kShadowMaskSpot</param>
float GetPunctualShadow(float4 svPosition, float3 worldPos, float3 lightPos, float3 N, uint shadowMask, uint typeBit,
                        float sourceRadius, uint lightIndex, uint softShadowLights, bool useScreen)
{
    if (useScreen && (shadowMask & typeBit) != 0)
    {
        uint slot = FindSoftShadowSlot(softShadowLights, typeBit == kShadowMaskSpot, lightIndex);
        if (slot < kSoftShadowSlotCount)
        {
            return gPunctualShadowTexture.Load(int3(int2(svPosition.xy), 0))[slot];
        }
    }
    return TraceShadowPunctual(worldPos, lightPos, N, shadowMask, typeBit, sourceRadius);
}

#endif // SCREEN_SPACE_SHADOW_HLSLI
