#ifndef PICKING_HLSLI
#define PICKING_HLSLI

// クリックされた画素のオブジェクト番号を拾うパスの共通部。VSは Object3d.VS を流用。
// 1回目(PickingDepth.PS)で手前の深度を求め、2回目(PickingID.PS)でその深度の断片だけが番号を書く
#include "Object3d.hlsli"

struct PickingBuffer
{
    int objID;
    uint depthBits; // 深度(float)のビット列。0〜1の正の値は、ビット列の大小が値の大小と一致する
};

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTextures[] : register(t0);

SamplerState gSampler : register(s0);
RWStructuredBuffer<PickingBuffer> gPickingBuffer : register(u0);

// マウスの位置とクリックしたか
cbuffer PickingData : register(b4)
{
    int2 pickingPixelCoord; // マウスの位置
    uint pickingEnable; // クリックしたか
};

cbuffer ObjIDData : register(b5)
{
    uint objID;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// <summary>この断片をクリックの対象にするか。しないものは捨てる</summary>
/// <remarks>見えていない物は拾わない。本描画が捨てる画素と、色のアルファが0で描いても見えない物</remarks>
void DiscardUnpickable(VertxShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float alpha = gTextures[gMaterial.textureIndex].Sample(gSampler, transformedUV.xy).a;

    if (alpha <= gMaterial.alphaRef || alpha * gMaterial.color.a <= 0.0f)
    {
        discard;
    }
    if (pickingEnable == 0 || objID == -1 || any(int2(input.position.xy) != pickingPixelCoord))
    {
        discard;
    }
}

/// <summary>アルファ0で出すので、アルファ合成のもとでは色は変わらない</summary>
PixelShaderOutput NoColor()
{
    PixelShaderOutput output;
    output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    return output;
}

#endif // PICKING_HLSLI
