#include "Object3d.hlsli"

// 画面空間のレイトレとデノイザ用に、ワールド法線と深度だけを書き出すパス。VSは Object3d.VS を流用。
// 法線マップを当てないのは、レイの原点浮かしとエッジ判定に要るのが幾何形状の向きだから

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTextures[] : register(t0);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 normal : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    // 本描画と同じ条件で捨てないと、抜けている所が塞がっているものとしてAOが出る
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float alpha = gTextures[gMaterial.textureIndex].Sample(gSampler, transformedUV.xy).a;

    if (alpha <= gMaterial.alphaRef)
    {
        discard;
    }

    PixelShaderOutput output;
    output.normal = float4(normalize(input.normal), 0.0f);
    return output;
}
