#ifndef SCREEN_PROBE_GI_HLSLI
#define SCREEN_PROBE_GI_HLSLI

// 前方描画から画面のプローブの間接光を引く。計算は Raytracing/ScreenProbe*.CS.hlsl

// rgb=間接光, a=使えるか(0〜1)
Texture2D<float4> gScreenGI : register(t2, space3);

/// <summary>この画素の間接光</summary>
float4 SampleScreenGI(float4 svPosition)
{
    return gScreenGI.Load(int3(int2(svPosition.xy), 0));
}

#endif // SCREEN_PROBE_GI_HLSLI
