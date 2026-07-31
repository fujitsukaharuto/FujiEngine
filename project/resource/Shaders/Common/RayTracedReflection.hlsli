#ifndef RAYTRACED_REFLECTION_HLSLI
#define RAYTRACED_REFLECTION_HLSLI

// 鏡面の環境光を、インラインレイトレで遮蔽しながら引く。
// TLAS と RayQuery は影と共有するので RayTracedShadow.hlsli を使う
#include "RayTracedShadow.hlsli"

// 環境キューブマップ。space0 は gTextures[]、space2 は TLAS が使用中なので space1。
// シェーダごとに宣言すると space がずれるので、必ずこのヘッダを include すること
TextureCube<float4> gEnvironment : register(t0, space1);

/// <summary>反射方向の環境色を返す。遮蔽されていれば occludedColor</summary>
/// <remarks>当たった先の色は出せないので、シーンの物体そのものは映らない</remarks>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="N">面の法線</param>
/// <param name="V">視線方向(面 → カメラ、正規化済み)</param>
/// <param name="occludedColor">遮蔽された向きに使う色</param>
/// <param name="maxDistance">この距離までの遮蔽物を見る</param>
/// <param name="enable">0なら遮蔽を見ずに環境色をそのまま返す</param>
float3 TraceReflection(float3 worldPos, float3 N, float3 V, float3 occludedColor,
                       float maxDistance, uint enable, SamplerState smp)
{
    float3 R = reflect(-V, N);
    float3 environmentColor = gEnvironment.Sample(smp, R).rgb;

    if (enable == 0 || maxDistance <= 0.0f)
    {
        return environmentColor;
    }

    // 遮られていれば空は見えない
    if (TraceShadowRay(worldPos, R, N, maxDistance) == 0.0f)
    {
        return occludedColor;
    }

    return environmentColor;
}

#endif // RAYTRACED_REFLECTION_HLSLI
