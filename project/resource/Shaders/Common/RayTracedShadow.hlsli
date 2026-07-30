#ifndef RAYTRACED_SHADOW_HLSLI
#define RAYTRACED_SHADOW_HLSLI

// インラインレイトレ(DXR 1.1 RayQuery)によるハードシャドウ。
// space0 は gTextures[] が無制限、space1 は gEnvironment が使用中なので space2 に固定する。
// TLAS を読むシェーダは必ずこのヘッダを include すること(個別宣言すると space がずれる)
RaytracingAccelerationStructure gSceneTLAS : register(t0, space2);

// 小さいと自分の面に当たって縞が出て、大きいと接地部が浮く
static const float kShadowRayBias = 0.02f;
static const float kShadowRayMaxDistance = 1000.0f;

/// <summary>遮蔽されていれば0、そうでなければ1を返す</summary>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="L">ライトへ向かう単位ベクトル</param>
/// <param name="N">面の法線</param>
/// <param name="enable">0なら常に1を返す</param>
float TraceShadow(float3 worldPos, float3 L, float3 N, uint enable)
{
    if (enable == 0)
    {
        return 1.0f;
    }

    RayDesc ray;
    ray.Origin = worldPos + N * kShadowRayBias;
    ray.Direction = L;
    ray.TMin = 0.0f;
    ray.TMax = kShadowRayMaxDistance;

    // 最近傍を探す必要が無いので、最初に当たった時点で打ち切る
    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
    q.Proceed();

    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.0f : 1.0f;
}

#endif // RAYTRACED_SHADOW_HLSLI
