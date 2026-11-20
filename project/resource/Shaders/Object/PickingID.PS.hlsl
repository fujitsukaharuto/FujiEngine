#include "Picking.hlsli"

// ピッキングの2回目。1回目で求めたいちばん手前の深度と一致した断片だけが番号を書く。
// 同じVSで同じ形状を描くので、深度は1回目とビット単位で一致する

PixelShaderOutput main(VertxShaderOutput input)
{
    DiscardUnpickable(input);

    if (asuint(input.position.z) == gPickingBuffer[0].depthBits)
    {
        gPickingBuffer[0].objID = objID;
        gPickingBuffer[0].meshIndex = gMaterial.meshIndex;
    }

    return NoColor();
}
