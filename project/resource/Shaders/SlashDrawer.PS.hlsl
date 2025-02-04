#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

//reflectionModel
struct Camera
{
    float3 worldPosition;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
};

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosStart;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionnalLight : register(b1);

//reflectionModel
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);



struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float2 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;
    transformedUV = clamp(transformedUV, 0.0f, 1.0f);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV);
    //if (textureColor.a == 0.0)
    //{
    //    discard;
    //}
    //if (textureColor.a <= 0.5)
    //{
    //    discard;
    //}
    
    //output.color = gMaterial.color * textureColor;
    //if (output.color.a == 0.0)
    //{
    //    discard;
    //}
    
    //return output;
    
    
    // 内側の炎のために縮小したUV
    float2 innerUV = float2(transformedUV.x, transformedUV.y * 0.46); // UVを縮小
    innerUV = clamp(innerUV, 0.1f, 1.0f);
    float4 textureColor2 = gTexture.Sample(gSampler, innerUV);

    // 内側の炎の色を固定
    float4 innerColor = float4(0.9f, 0.35f, 0.0f, textureColor2.a);

    // 透過チェック
    if (textureColor.a == 0.0 && textureColor2.a == 0.0)
    {
        discard;
    }

    //// 外側と内側のブレンド係数を計算 (アルファ値による滑らかな変化)
    //float blendFactor = smoothstep(0.4, 0.8, textureColor.a);
    
    //// 外側のテクスチャと内側の固定色をブレンド
    //float4 finalColor = lerp(textureColor, innerColor, blendFactor);
    
    float4 finalColor = textureColor;
    if (innerColor.a != 0.0)
    {
        output.color = innerColor;
    }
    else
    {
        output.color = gMaterial.color * finalColor;
    }

    if (output.color.a == 0.0)
    {
        discard;
    }

    return output;
    
}