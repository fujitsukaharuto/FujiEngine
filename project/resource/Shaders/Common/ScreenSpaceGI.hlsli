#ifndef SCREEN_SPACE_GI_HLSLI
#define SCREEN_SPACE_GI_HLSLI

// 前方描画から画面の照り返し (SSGI) を引く。計算は Raytracing/ScreenSpaceGI*.CS.hlsl

// rgb=照り返し, a=遮られた割合
Texture2D<float4> gScreenSpaceGI : register(t5, space3);

/// <summary>この画素の照り返し</summary>
float4 SampleScreenSpaceGI(float4 svPosition)
{
    return gScreenSpaceGI.Load(int3(int2(svPosition.xy), 0));
}

#endif // SCREEN_SPACE_GI_HLSLI
