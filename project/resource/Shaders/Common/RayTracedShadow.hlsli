#ifndef RAYTRACED_SHADOW_HLSLI
#define RAYTRACED_SHADOW_HLSLI

// インラインレイトレ(DXR 1.1 RayQuery)による影。
// space0 は gTextures[] が無制限、space1 は gEnvironment が使用中なので space2 に固定する。
// TLAS を読むシェーダは必ずこのヘッダを include すること(個別宣言すると space がずれる)
#include "Sampling.hlsli"

RaytracingAccelerationStructure gSceneTLAS : register(t0, space2);

// 小さいと自分の面に当たって縞が出て、大きいと接地部が浮く
static const float kShadowRayBias = 0.02f;
static const float kShadowRayMaxDistance = 1000.0f;

// AllLights::rayTracedShadowMask のビット割り当て。C++ 側 (LightManager.h) と同じ値であること
static const uint kShadowMaskDirectional = 1u << 0;
static const uint kShadowMaskPoint = 1u << 1;
static const uint kShadowMaskSpot = 1u << 2;

// AllLights::shadowMode の値。C++ 側 (LightManager.h の RayTracedShadowMode) と同じ値であること
static const uint kShadowModeHard = 0; // 前方描画のPSで1本だけ飛ばす。輪郭が硬い代わりに軽い
static const uint kShadowModeSoft = 1; // 別パスで光源の広がりぶん散らして飛ばし、デノイズする

/// <summary>指定した距離までの遮蔽を調べる。遮蔽されていれば0、そうでなければ1を返す</summary>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="L">ライトへ向かう単位ベクトル</param>
/// <param name="N">面の法線</param>
/// <param name="maxDistance">この距離より遠くの遮蔽物は無視する</param>
float TraceShadowRay(float3 worldPos, float3 L, float3 N, float maxDistance)
{
    RayDesc ray;
    ray.Origin = worldPos + N * kShadowRayBias;
    ray.Direction = L;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;

    // 最近傍を探す必要が無いので、最初に当たった時点で打ち切る
    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
    q.Proceed();

    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.0f : 1.0f;
}

/// <summary>平行光源の影。光源に位置が無いので遮蔽物は無限遠まで探す</summary>
/// <param name="L">ライトへ向かう単位ベクトル</param>
/// <param name="shadowMask">AllLights::rayTracedShadowMask</param>
float TraceShadowDirectional(float3 worldPos, float3 L, float3 N, uint shadowMask)
{
    if ((shadowMask & kShadowMaskDirectional) == 0)
    {
        return 1.0f;
    }

    return TraceShadowRay(worldPos, L, N, kShadowRayMaxDistance);
}

/// <summary>平行光源のソフトシャドウ。光源の見かけの大きさぶんレイを円錐状に散らす</summary>
/// <remarks>
/// 本数がそのまま負荷なので、少ない本数で散らしてデノイザで均す前提。
/// 半影の広がりは遮蔽物までの距離に比例するが、これは角度で散らすだけで自然にそうなる
/// </remarks>
/// <param name="L">ライトへ向かう単位ベクトル。円錐の中心になる</param>
/// <param name="angularRadius">光源の見かけの半径(ラジアン)。太陽で約0.0047</param>
/// <param name="sampleCount">飛ばす本数</param>
/// <param name="rotationSeed">散らし方を回す種。[0,1)</param>
float TraceSoftShadowDirectional(float3 worldPos, float3 L, float3 N,
                                 float angularRadius, uint sampleCount, float rotationSeed)
{
    if (sampleCount == 0)
    {
        return 1.0f;
    }
    // 散らす意味が無いなら円錐を組む手間を掛けずに1本で済ませる
    if (sampleCount == 1 || angularRadius <= 0.0f)
    {
        return TraceShadowRay(worldPos, L, N, kShadowRayMaxDistance);
    }

    float3 tangent;
    float3 bitangent;
    BuildOrthonormalBasis(L, tangent, bitangent);

    float angle = rotationSeed * kSamplingTwoPI;
    float cosA = cos(angle);
    float sinA = sin(angle);
    // 円錐の底面は L の先にある単位距離の円板なので、半径は角度の tan になる
    float coneRadius = tan(angularRadius);

    float visible = 0.0f;
    for (uint i = 0; i < sampleCount; i++)
    {
        float2 disk = UniformDiskSample(Hammersley(i, sampleCount), cosA, sinA) * coneRadius;
        float3 dir = normalize(L + disk.x * tangent + disk.y * bitangent);

        // 面より下を向いた向きは光源のその部分が地平線の下にある＝自分自身に遮られている。
        // レイを飛ばすと原点をずらしたぶんだけ通り抜けてしまうので、飛ばさずに0を数える
        if (dot(dir, N) <= 0.0f)
        {
            continue;
        }

        visible += TraceShadowRay(worldPos, dir, N, kShadowRayMaxDistance);
    }

    return visible / sampleCount;
}

/// <summary>点光源/スポットライトの影</summary>
/// <remarks>無限遠まで探すと光源より奥の物が遮蔽物と判定されるので、光源までの距離で打ち切る</remarks>
/// <param name="lightPos">光源のワールド座標</param>
/// <param name="shadowMask">AllLights::rayTracedShadowMask</param>
/// <param name="typeBit">kShadowMaskPoint か kShadowMaskSpot</param>
float TraceShadowPunctual(float3 worldPos, float3 lightPos, float3 N, uint shadowMask, uint typeBit)
{
    if ((shadowMask & typeBit) == 0)
    {
        return 1.0f;
    }

    float3 toLight = lightPos - worldPos;
    float distance = length(toLight);
    // バイアスで浮かせた分より光源が近い＝間に遮蔽物が入る余地が無い
    if (distance <= kShadowRayBias)
    {
        return 1.0f;
    }

    // 光源のすぐ手前で止める。光源と同じ位置にある面を自分の遮蔽物と数えないため
    return TraceShadowRay(worldPos, toLight / distance, N, distance - kShadowRayBias);
}

#endif // RAYTRACED_SHADOW_HLSLI
