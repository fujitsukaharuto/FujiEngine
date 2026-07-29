// コンピュート・スプラット用の共有定義
// 蓄積バッファは 1ピクセルあたり4つのuint(R,G,B,予備)を固定小数(×255)で持つ。

struct SplatParam
{
    uint2 dims;           // 描画解像度 (width, height)
    uint enableDepthTest; // 1=シーン遮蔽の深度テスト有効(フル解像度時のみ)。粒子同士のテストはしない
    uint pad;
};

static const uint kSplatScale = 255; // 固定小数スケール(色0..1を整数化)

// ピクセル(x,y) のチャンネル先頭インデックス
uint SplatPixelBase(uint x, uint y, uint width)
{
    return (y * width + x) * 4;
}
