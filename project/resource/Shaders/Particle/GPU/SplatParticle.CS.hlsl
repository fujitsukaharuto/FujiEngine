#include "CSParticle.hlsli"
#include "Splat.hlsli"

// 生存パーティクルを画面へ点描(atomic加算)するコンピュート・ラスタライザ。
// パーティクルバッファはUAV状態のまま読むため RWStructuredBuffer で受ける
RWStructuredBuffer<uint> gSplatAccum : register(u0);
RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u1);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u2);
RWStructuredBuffer<Particle_Scale> gParticles_Scale : register(u3);

struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};
ConstantBuffer<PerView> gPerView : register(b0);
ConstantBuffer<SplatParam> gSplatParam : register(b1);

// シーン不透明深度(NDC深度[0,1], 標準: 大きいほど遠い)。フル解像度時のみ遮蔽テストに使用。
Texture2D<float> gSceneDepth : register(t0);

// この半径(px)以下は1pxにまとめる。丸のループは画素中心を拾うので、これより小さいと1つも拾えないことがある
static const float kPointRadius = 0.75f;
// 丸の重み (1 - t^2)^2 を円盤上で積分した値は π r^2 / 3
static const float kDiscEnergy = 3.14159265f / 3.0f;

uint Hash(uint v)
{
    v ^= v >> 16;
    v *= 0x7feb352d;
    v ^= v >> 15;
    v *= 0x846ca68b;
    v ^= v >> 16;
    return v;
}

// 1px分の寄与を蓄積バッファへ足す。端数は確率的に切り上げるので、1/kSplatScale より暗い寄与も平均では残る
void AddPixel(uint px, uint py, float3 value, uint seed)
{
    uint h = Hash(seed ^ (px * 0x9E3779B9u) ^ (py * 0x85EBCA6Bu));
    float3 rnd = float3(h & 0x3FF, (h >> 10) & 0x3FF, (h >> 20) & 0x3FF) / 1024.0f;
    uint3 add = (uint3) (value * (float) kSplatScale + rnd);

    uint base = SplatPixelBase(px, py, gSplatParam.dims.x);
    if (add.x > 0)
        InterlockedAdd(gSplatAccum[base + 0], add.x);
    if (add.y > 0)
        InterlockedAdd(gSplatAccum[base + 1], add.y);
    if (add.z > 0)
        InterlockedAdd(gSplatAccum[base + 2], add.z);
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = GetParticleIndex(DTid); // DTid.x + DTid.y * kThreadsPerRow
    if (particleIndex >= kMaxParticles)
    {
        return;
    }

    // 死んでいる粒子はcolorだけ読んで早期終了(帯域節約)
    uint colorPacked = gParticles_Color[particleIndex].color;
    float4 color = UnpackRGBA8(colorPacked);
    if (color.a <= 0.0f)
    {
        return;
    }

    // ワールド座標 → クリップ → NDC → ピクセル
    float3 wpos = UnpackPos21(gParticles_Trans[particleIndex].packedPos);
    float4 clip = mul(float4(wpos, 1.0f), gPerView.viewProjection);
    if (clip.w <= 0.0001f)
    {
        return; // カメラ背面
    }
    float2 ndc = clip.xy / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f)
    {
        return; // 画面外(視錐台カリング相当)
    }

    float2 dims = float2(gSplatParam.dims);
    float2 uv = ndc * float2(0.5f, -0.5f) + 0.5f;
    float2 center = uv * dims; // ピクセル座標(画素中心は +0.5)
    uint px = (uint) center.x;
    uint py = (uint) center.y;
    if (px >= gSplatParam.dims.x || py >= gSplatParam.dims.y)
    {
        return;
    }

    float particleDepth = clip.z / clip.w; // NDC深度[0,1](標準: 大=遠)
    bool depthTest = gSplatParam.enableDepthTest != 0;

    // 加算ブレンド相当: 寄与 = rgb * alpha (ラスタ版の SrcAlpha/One と一致)
    float3 value = color.rgb * color.a;

    // 画面上の半径(px)。フットプリント無効時は読まずに 1粒子=1px のまま描く
    float radius = 0.0f;
    float drawRadius = 0.0f;
    if (gSplatParam.enableFootprint != 0)
    {
        float scale = UnpackHalf2(gParticles_Scale[particleIndex].packedScale).x;
        // カメラの上方向へ scale だけずらした点との画面上の距離
        float4 clipOffset = mul(float4(gPerView.billboardMatrix[1].xyz * scale, 0.0f), gPerView.viewProjection);
        radius = length(clipOffset.xy / clip.w * 0.5f * dims);
        drawRadius = min(radius, gSplatParam.maxRadius);
    }

    if (drawRadius <= kPointRadius)
    {
        // シーン遮蔽の深度テスト(フル解像度時のみ。粒子同士のテストはしない)
        if (depthTest && particleDepth > gSceneDepth.Load(int3(px, py, 0)))
        {
            return; // 不透明面より奥 → 遮蔽されるので描かない
        }
        if (gSplatParam.enableFootprint != 0)
        {
            // 丸で描いたときと同じ総量を1pxへ入れる(1px未満の粒子は面積ぶん暗くなる)。
            // 上限で絞られた大きい粒子は 1 で止め、1粒子1px のときより明るくはしない
            value *= min(1.0f, kDiscEnergy * radius * radius);
        }
        AddPixel(px, py, value, particleIndex);
        return;
    }

    // 画素中心が半径内に入る範囲
    int2 lo = max((int2) ceil(center - drawRadius - 0.5f), int2(0, 0));
    int2 hi = min((int2) floor(center + drawRadius - 0.5f), (int2) gSplatParam.dims - 1);
    float invRadiusSq = 1.0f / (drawRadius * drawRadius);

    // 重みの合計を先に取り、総量が (π r^2 / 3) になるよう正規化する(画素中心の拾い方で明るさが揺れるのを防ぐ)。
    // ただし倍率は 1 で止める。上限で半径を絞られた大きい粒子へ総量を詰め込むと、中心が何十倍にも光って白く潰れる
    float weightSum = 0.0f;
    int x, y;
    for (y = lo.y; y <= hi.y; y++)
    {
        for (x = lo.x; x <= hi.x; x++)
        {
            float2 d = float2(x, y) + 0.5f - center;
            float t = saturate(1.0f - dot(d, d) * invRadiusSq);
            weightSum += t * t;
        }
    }
    if (weightSum <= 0.0f)
    {
        return;
    }
    value *= min(1.0f, kDiscEnergy * radius * radius / weightSum);

    for (y = lo.y; y <= hi.y; y++)
    {
        for (x = lo.x; x <= hi.x; x++)
        {
            float2 d = float2(x, y) + 0.5f - center;
            float t = saturate(1.0f - dot(d, d) * invRadiusSq);
            if (t <= 0.0f)
            {
                continue;
            }
            if (depthTest && particleDepth > gSceneDepth.Load(int3(x, y, 0)))
            {
                continue;
            }
            AddPixel((uint) x, (uint) y, value * (t * t), particleIndex);
        }
    }
}
