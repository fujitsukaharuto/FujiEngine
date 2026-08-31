#ifndef SCREEN_SPACE_AO_HLSLI
#define SCREEN_SPACE_AO_HLSLI

// 前方描画からAOを引くためのヘッダ。経路(kAOMode*)の切り替えもここに閉じ込める。
#include "RayTracedAO.hlsli"

// 別パスで計算しデノイズ済みのAO。space0=gTextures[] / space1=gEnvironment / space2=TLAS の空き
Texture2D<float> gAOTexture : register(t0, space3);

/// <summary>この画素のAOを引く。1で開けている、0で完全に囲まれている</summary>
/// <remarks>AOバッファは画面と同じ解像度で画素が一対一に対応するので、SV_POSITION を整数化して引ける</remarks>
float SampleScreenAO(float4 svPosition)
{
    return gAOTexture.Load(int3(int2(svPosition.xy), 0));
}

/// <summary>設定された経路でAOを求める</summary>
/// <remarks>Screen が本命。Inline は絵が変わっていないかの切り分け用に残してある</remarks>
/// <param name="svPosition">ピクセルシェーダの SV_POSITION</param>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="N">面の法線</param>
/// <param name="mode">AllLights::aoMode</param>
float GetAmbientOcclusion(float4 svPosition, float3 worldPos, float3 N,
                          uint mode, uint sampleCount, float radius, float intensity)
{
    if (mode == kAOModeScreen)
    {
        return SampleScreenAO(svPosition);
    }

    if (mode == kAOModeInline)
    {
        // 時間に依存しない種＝同じ点なら常に同じ結果。単体でちらつかない代わりに空間のノイズが残る
        return TraceAO(worldPos, N, sampleCount, radius, intensity, rand3dTo1d(worldPos));
    }

    return 1.0f;
}

#endif // SCREEN_SPACE_AO_HLSLI
