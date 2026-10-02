// コンピュート・スプラット用の共有定義
// 蓄積バッファは 1ピクセルあたり4つのuint(R,G,B,予備)を固定小数(×kSplatScale)で持つ。

struct SplatParam
{
    uint2 dims;           // 描画解像度 (width, height)
    uint enableDepthTest; // 1=シーン遮蔽の深度テスト有効(フル解像度時のみ)。粒子同士のテストはしない
    float intensity;      // 合成時に掛ける明るさ(1より大きくするとHDR域へ出てブルームに乗る)
    uint enableFootprint; // 1=粒子のスケールを画面上の半径に反映した丸で描く / 0=1粒子1px
    float maxRadius;      // 丸の半径の上限(px)
    uint2 pad;
};

// 固定小数スケール。1ピクセルに重なれる上限は 2^32 / kSplatScale (全白で約420万粒子)
static const uint kSplatScale = 1024;

// ピクセル(x,y) のチャンネル先頭インデックス
uint SplatPixelBase(uint x, uint y, uint width)
{
    return (y * width + x) * 4;
}
