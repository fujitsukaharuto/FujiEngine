#include "CSParticle.hlsli"
#include "Splat.hlsli"

// 生存パーティクルを画面へ点描(atomic加算)するコンピュート・ラスタライザ。
// パーティクルバッファはUAV状態のまま読むため RWStructuredBuffer で受ける
RWStructuredBuffer<uint> gSplatAccum : register(u0);
RWStructuredBuffer<Particle_Translate> gParticles_Trans : register(u1);
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u2);

struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};
ConstantBuffer<PerView> gPerView : register(b0);
ConstantBuffer<SplatParam> gSplatParam : register(b1);

// シーン不透明深度(NDC深度[0,1], 標準: 大きいほど遠い)。フル解像度時のみ遮蔽テストに使用。
Texture2D<float> gSceneDepth : register(t0);

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

    float2 uv = ndc * float2(0.5f, -0.5f) + 0.5f;
    uint px = (uint) (uv.x * (float) gSplatParam.dims.x);
    uint py = (uint) (uv.y * (float) gSplatParam.dims.y);
    if (px >= gSplatParam.dims.x || py >= gSplatParam.dims.y)
    {
        return;
    }

    // シーン遮蔽の深度テスト(フル解像度時のみ。粒子同士のテストはしない)
    if (gSplatParam.enableDepthTest != 0)
    {
        float particleDepth = clip.z / clip.w; // NDC深度[0,1](標準: 大=遠)
        float sceneDepth = gSceneDepth.Load(int3(px, py, 0));
        if (particleDepth > sceneDepth)
        {
            return; // 不透明面より奥 → 遮蔽されるので描かない
        }
    }

    // 加算ブレンド相当: 寄与 = rgb * alpha (ラスタ版の SrcAlpha/One と一致)
    uint base = SplatPixelBase(px, py, gSplatParam.dims.x);
    uint3 add = (uint3) (color.rgb * color.a * (float) kSplatScale + 0.5f);
    if (add.x > 0)
        InterlockedAdd(gSplatAccum[base + 0], add.x);
    if (add.y > 0)
        InterlockedAdd(gSplatAccum[base + 1], add.y);
    if (add.z > 0)
        InterlockedAdd(gSplatAccum[base + 2], add.z);
}
