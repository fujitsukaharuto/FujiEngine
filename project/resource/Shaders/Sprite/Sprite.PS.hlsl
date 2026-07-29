#include "../Object/Object3d.hlsli"

// Material 構造体は Object3d.hlsli の共有定義を使う(独自に書き写すとレイアウトが分岐するため)

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    if (textureColor.a == 0.0)
    {
        discard;
    }
    if (textureColor.a <= gMaterial.alphaRef)
    {
        discard;
    }
    
    // テクスチャは TextureManager が WIC_FLAGS_FORCE_SRGB で読み SRV も _SRGB なので、
    // サンプルした時点でハードウェアがリニア化済み。ここでガンマを掛けると二重デコードになる
    output.color = gMaterial.color * textureColor;
    if (output.color.a == 0.0)
    {
        discard;
    }
    
	return output;
}