#include "Picking.hlsli"

// ピッキングの1回目。クリックされた画素でいちばん手前の深度を求める。
// 最小値の更新は不可分な演算なので、描画の順序に依らず同じ結果になる

PixelShaderOutput main(VertxShaderOutput input)
{
    DiscardUnpickable(input);

    InterlockedMin(gPickingBuffer[0].depthBits, asuint(input.position.z));

    return NoColor();
}
