#ifndef RAYTRACED_SHADOW_HLSLI
#define RAYTRACED_SHADOW_HLSLI

// インラインレイトレ(DXR 1.1 RayQuery)による影。TLAS は space2 に固定する
// (space0 は gTextures[] が無制限、space1 は gEnvironment が使用中)
#include "Sampling.hlsli"
// RAY_CUTOUT_SHADOW を定義して include すると抜きのある物の影が抜ける (gRayInstances と gRayTextures[] が要る)
#ifdef RAY_CUTOUT_SHADOW
#include "RayInstance.hlsli"
#endif

RaytracingAccelerationStructure gSceneTLAS : register(t0, space2);

// 小さいと自分の面に当たって縞が出て、大きいと接地部が浮く
static const float kShadowRayBias = 0.02f;
static const float kShadowRayMaxDistance = 1000.0f;

// 深度から復元した位置の粗さ。深度の刻み 6e-7 * z^2 と、逆行列で戻すときの丸め 2e-6 * z^2 の合計。
// near=0.1 での値。near を変えたら見直すこと
static const float kDepthReconstructionError = 4.0e-6f;

// AllLights::rayTracedShadowMask のビット割り当て。C++ 側 (LightManager.h) と同じ値であること
static const uint kShadowMaskDirectional = 1u << 0;
static const uint kShadowMaskPoint = 1u << 1;
static const uint kShadowMaskSpot = 1u << 2;

// インスタンスの InstanceMask。C++ 側 (RaytracingScene.h の kInstanceMask*) と同じ値であること
static const uint kRayMaskOpaque = 0x01;      // 不透明
static const uint kRayMaskTranslucent = 0x02; // 半透明。薄い影は TraceTranslucentTransmittance で別に求める
static const uint kRayMaskStaticOpaque = 0x04; // 動かない不透明

// AllLights::shadowMode の値。C++ 側 (LightManager.h の RayTracedShadowMode) と同じ値であること
static const uint kShadowModeHard = 0; // 前方描画のPSで1本だけ飛ばす。輪郭が硬い代わりに軽い
static const uint kShadowModeSoft = 1; // 別パスで光源の広がりぶん散らして飛ばし、デノイズする

/// <summary>指定した距離までの遮蔽を調べる。遮蔽されていれば0、そうでなければ1を返す</summary>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="L">ライトへ向かう単位ベクトル</param>
/// <param name="N">面の法線</param>
/// <param name="maxDistance">この距離より遠くの遮蔽物は無視する</param>
/// <param name="bias">原点を法線方向へ浮かせる量</param>
/// <param name="instanceMask">当たりにするインスタンスの InstanceMask</param>
float TraceShadowRayMasked(float3 worldPos, float3 L, float3 N, float maxDistance, float bias, uint instanceMask)
{
    RayDesc ray;
    ray.Origin = worldPos + N * bias;
    ray.Direction = L;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;

    // 最近傍を探す必要が無いので、最初に当たった時点で打ち切る
    // 半透明は影と AO では見えないものとして飛ばす
    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, instanceMask, ray);
    // 抜きのある物は候補として戻る。抜けていなければ当たり
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
#ifdef RAY_CUTOUT_SHADOW
            if (RayCutoutIsSolid(q.CandidateInstanceID() + q.CandidateGeometryIndex(), q.CandidatePrimitiveIndex(),
                                 q.CandidateTriangleBarycentrics()))
#endif
            {
                q.CommitNonOpaqueTriangleHit();
            }
        }
    }

    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.0f : 1.0f;
}

/// <summary>不透明な物すべてを遮蔽物として調べる</summary>
float TraceShadowRayBiased(float3 worldPos, float3 L, float3 N, float maxDistance, float bias)
{
    return TraceShadowRayMasked(worldPos, L, N, maxDistance, bias, kRayMaskOpaque);
}

#ifdef RAY_CUTOUT_SHADOW
/// <summary>半透明の物を通って届く光の割合 (色つき)。不透明な物による遮りは含まない</summary>
/// <remarks>1枚ごとに (1 - a) × lerp(1, 色, a) を掛ける。表の面だけを数える</remarks>
float3 TraceTranslucentTransmittance(float3 worldPos, float3 L, float3 N, float maxDistance)
{
    RayDesc ray;
    ray.Origin = worldPos + N * kShadowRayBias;
    ray.Direction = L;
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;

    float3 transmittance = float3(1.0f, 1.0f, 1.0f);
    // 全部の候補を数えるので、どれも当たりにしない
    RayQuery < RAY_FLAG_FORCE_NON_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kRayMaskTranslucent, ray);
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE && q.CandidateTriangleFrontFace())
        {
            RayInstance instance = gRayInstances[q.CandidateInstanceID() + q.CandidateGeometryIndex()];
            float opacity = saturate(instance.opacity);
            transmittance *= (1.0f - opacity) * lerp(float3(1.0f, 1.0f, 1.0f), saturate(instance.color), opacity);
            if (all(transmittance < 1e-3f))
            {
                q.Abort();
            }
        }
    }
    return transmittance;
}
#endif

/// <summary>補間された正確な位置から飛ばすとき用。浮かせる量は固定</summary>
float TraceShadowRay(float3 worldPos, float3 L, float3 N, float maxDistance)
{
    return TraceShadowRayBiased(worldPos, L, N, maxDistance, kShadowRayBias);
}

/// <summary>深度から復元した位置から飛ばすときの浮かせる量</summary>
/// <remarks>復元した位置は遠いほど粗い。距離およそ70から先だけ kShadowRayBias より大きくなる</remarks>
/// <param name="viewDistance">カメラからその点までの距離</param>
float ReconstructedRayBias(float viewDistance)
{
    return max(kShadowRayBias, kDepthReconstructionError * viewDistance * viewDistance);
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
/// <remarks>本数がそのまま負荷なので、少ない本数で散らしてデノイザで均す前提</remarks>
/// <param name="L">ライトへ向かう単位ベクトル。円錐の中心になる</param>
/// <param name="angularRadius">光源の見かけの半径(ラジアン)。太陽で約0.0047</param>
/// <param name="sampleCount">飛ばす本数</param>
/// <param name="rotationSeed">散らし方を回す種。[0,1)</param>
/// <param name="bias">原点を法線方向へ浮かせる量</param>
float TraceSoftShadowDirectional(float3 worldPos, float3 L, float3 N,
                                 float angularRadius, uint sampleCount, float rotationSeed, float bias)
{
    if (sampleCount == 0)
    {
        return 1.0f;
    }
    // 散らす意味が無いなら円錐を組む手間を掛けずに1本で済ませる
    if (sampleCount == 1 || angularRadius <= 0.0f)
    {
        return TraceShadowRayBiased(worldPos, L, N, kShadowRayMaxDistance, bias);
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
    for (uint k = 0; k < sampleCount; k++)
    {
        // 離れた2本が両方とも当たるか両方とも外れれば、半影ではないとみなして残りを飛ばさない
        uint i = (k == 1) ? sampleCount / 2 : ((k != 0 && k <= sampleCount / 2) ? k - 1 : k);
        if (k == 2 && (visible == 0.0f || visible == 2.0f))
        {
            return visible * 0.5f;
        }
        float2 disk = UniformDiskSample(Hammersley(i, sampleCount), cosA, sinA) * coneRadius;
        float3 dir = normalize(L + disk.x * tangent + disk.y * bitangent);

        // 面より下を向いた向きは光源のその部分が地平線の下にある＝自分自身に遮られている。
        // レイを飛ばすと原点をずらしたぶんだけ通り抜けてしまうので、飛ばさずに0を数える
        if (dot(dir, N) <= 0.0f)
        {
            continue;
        }

        visible += TraceShadowRayBiased(worldPos, dir, N, kShadowRayMaxDistance, bias);
    }

    return visible / sampleCount;
}

/// <summary>点光源/スポットライトの影</summary>
/// <remarks>無限遠まで探すと光源より奥の物が遮蔽物と判定されるので、光源までの距離で打ち切る</remarks>
/// <param name="lightPos">光源のワールド座標</param>
/// <param name="shadowMask">AllLights::rayTracedShadowMask</param>
/// <param name="typeBit">kShadowMaskPoint か kShadowMaskSpot</param>
/// <param name="sourceRadius">光源の大きさ。レイをこの手前で止める(光っている物の中心に置いた光源が、その物自身に遮られないように)</param>
float TraceShadowPunctual(float3 worldPos, float3 lightPos, float3 N, uint shadowMask, uint typeBit, float sourceRadius)
{
    if ((shadowMask & typeBit) == 0)
    {
        return 1.0f;
    }

    float3 toLight = lightPos - worldPos;
    float distance = length(toLight);
    // 光源のすぐ手前で止める。光源と同じ位置にある面を自分の遮蔽物と数えないため
    float stopBefore = max(kShadowRayBias, sourceRadius);
    // 止める位置より光源が近い＝間に遮蔽物が入る余地が無い
    if (distance <= stopBefore)
    {
        return 1.0f;
    }

    return TraceShadowRay(worldPos, toLight / distance, N, distance - stopBefore);
}

#endif // RAYTRACED_SHADOW_HLSLI
