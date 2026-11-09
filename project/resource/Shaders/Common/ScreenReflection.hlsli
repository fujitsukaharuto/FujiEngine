#ifndef SCREEN_REFLECTION_HLSLI
#define SCREEN_REFLECTION_HLSLI

// 前方描画から画面の映り込みを引く。計算は Raytracing/ScreenReflection*.CS.hlsl

// rgb=当たった物の明るさ (空は0)、a=0.5+空が見えた割合/2。a=0 なら使えない
Texture2D<float4> gScreenReflection : register(t3, space3);

/// <summary>この画素の映り込み</summary>
float4 SampleScreenReflection(float4 svPosition)
{
    return gScreenReflection.Load(int3(int2(svPosition.xy), 0));
}

/// <summary>空が見えた割合(0〜1)。SampleScreenReflection の a から戻す</summary>
float ScreenReflectionSkyVisibility(float4 reflection)
{
    return saturate(reflection.a * 2.0f - 1.0f);
}

#endif // SCREEN_REFLECTION_HLSLI
