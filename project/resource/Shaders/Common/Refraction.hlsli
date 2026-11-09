#ifndef REFRACTION_HLSLI
#define REFRACTION_HLSLI

// 半透明の屈折。画面の写し (RefractionCapture) を屈折した向きにずらして引く

// space3 は画面空間の計算済みバッファ
Texture2D<float4> gRefractionScene : register(t4, space3);

// RefractionCapture.h の RefractionParams と並びが一致していること
cbuffer RefractionParams : register(b5)
{
    float4x4 refractionViewProjection;
    float refractionStrength; // 面の奥のこの距離(m)にある物を引く
    float refractionIOR;      // 屈折率
    uint refractionEnabled;   // 0 なら屈折させない
    float refractionPad;
};

/// <summary>面の向こうに見える物の色</summary>
/// <param name="svPosition">この画素の SV_POSITION</param>
float3 SampleRefraction(float3 worldPos, float3 normal, float3 toEye, float4 svPosition, SamplerState smp)
{
    float2 size;
    gRefractionScene.GetDimensions(size.x, size.y);
    float2 baseUV = svPosition.xy / size;

    // 全反射なら真っすぐ抜けたことにする
    float3 refracted = refract(-toEye, normal, 1.0f / refractionIOR);
    if (dot(refracted, refracted) < 1e-6f)
    {
        refracted = -toEye;
    }
    float4 clip = mul(float4(worldPos + refracted * refractionStrength, 1.0f), refractionViewProjection);
    float2 uv = baseUV;
    if (clip.w > 1e-4f)
    {
        uv = float2(clip.x / clip.w * 0.5f + 0.5f, 0.5f - clip.y / clip.w * 0.5f);
    }
    // 画面の端ほど元の位置へ寄せる
    float2 edge = saturate(min(uv, 1.0f - uv) * 20.0f);
    uv = lerp(baseUV, uv, edge.x * edge.y);
    return gRefractionScene.SampleLevel(smp, clamp(uv, 0.5f / size, 1.0f - 0.5f / size), 0.0f).rgb;
}

#endif // REFRACTION_HLSLI
