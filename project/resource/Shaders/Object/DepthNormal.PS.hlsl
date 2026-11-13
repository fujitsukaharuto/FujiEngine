#include "Object3d.hlsli"
#include "../Common/Wetness.hlsli"
#include "../Common/FlowingWater.hlsli"

// 画面空間のレイトレとデノイザ用に、ワールド法線と深度だけを書き出すパス。VSは Object3d.VS を流用。
// 法線は幾何形状の向き。a には画面の映り込みが使う粗さを入れ、映り込みを付けていない物は2を足す

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTextures[] : register(t0);

SamplerState gSampler : register(s0);
// 濡れた路面の設定だけ読む
ConstantBuffer<AllLights> gLights : register(b2);

struct PixelShaderOutput
{
    float4 normal : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input, bool isFrontFace : SV_IsFrontFace)
{
    // 本描画と同じ条件で捨てないと、抜けている所が塞がっているものとしてAOが出る
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float alpha = gTextures[gMaterial.textureIndex].Sample(gSampler, transformedUV.xy).a;

    if (alpha <= gMaterial.alphaRef)
    {
        discard;
    }

    PixelShaderOutput output;
    float roughness = saturate(gMaterial.roughness);
    // 両面の素材を裏から見ているときは法線を見ている側へ向ける
    float3 normal = normalize(input.normal);
    if (gMaterial.isTwoSided != 0 && !isFrontFace)
    {
        normal = -normal;
    }
    // 濡れた面は前方描画と同じ粗さにする (屋根の下かどうかは見ない)
    float environmentCoefficient = gMaterial.environmentCoefficient;
    if (gLights.wetness > 0.0f)
    {
        WetSurface wetSurface = GetWetSurface(input.WorldPosition, normal, gLights.wetness, gLights.puddleAmount, gLights.puddleScale);
        roughness = WetRoughness(wetSurface, roughness);
        // 波紋も前方描画と同じに立てる
        float3 flatNormal = normal;
        ApplyRainRipples(wetSurface, input.WorldPosition, flatNormal, gLights.rainAmount, gLights.fog.cloudTime, normal);
        environmentCoefficient = max(environmentCoefficient, wetSurface.wet);
    }
    // 流れる水のさざ波も前方描画と同じに立てる
    ApplyFlowingWater(gMaterial.flowSpeed, transformedUV.xy, input.WorldPosition, gLights.fog.cloudTime, normal);
    output.normal = float4(normal, environmentCoefficient > 0.0f ? roughness : roughness + 2.0f);
    return output;
}
