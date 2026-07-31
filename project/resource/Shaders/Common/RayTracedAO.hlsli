#ifndef RAYTRACED_AO_HLSLI
#define RAYTRACED_AO_HLSLI

// インラインレイトレによるアンビエントオクルージョン。
// TLAS と RayQuery は影と共有するので、宣言はこちらに持たず RayTracedShadow.hlsli を使う
#include "RayTracedShadow.hlsli"
#include "Random.hlsli"

static const float kAOTwoPI = 6.28318530718f;

/// <summary>法線から接空間の基底を作る(Duff et al. の分岐なし版)</summary>
void BuildOrthonormalBasis(float3 n, out float3 tangent, out float3 bitangent)
{
    float s = (n.z >= 0.0f) ? 1.0f : -1.0f;
    float a = -1.0f / (s + n.z);
    float c = n.x * n.y * a;
    tangent = float3(1.0f + s * n.x * n.x * a, s * c, -s * n.x);
    bitangent = float3(c, s + n.y * n.y * a, -n.y);
}

/// <summary>van der Corput 列。Hammersley 点の2次元目に使う</summary>
float RadicalInverse(uint i)
{
    return float(reversebits(i)) * 2.3283064365386963e-10f;
}

/// <summary>遮蔽されていない割合を返す。1で開けている、0で完全に囲まれている</summary>
/// <remarks>cosine 重みなので重みが分布に入り、最後は当たった本数を割るだけでよい</remarks>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="N">面の法線</param>
/// <param name="sampleCount">飛ばすレイの本数</param>
/// <param name="radius">この距離までの遮蔽物だけを見る</param>
/// <param name="intensity">0で無効、1で素の遮蔽率</param>
/// <param name="enable">0なら常に1を返す</param>
float TraceAO(float3 worldPos, float3 N, uint sampleCount, float radius, float intensity, uint enable)
{
    if (enable == 0 || sampleCount == 0 || radius <= 0.0f)
    {
        return 1.0f;
    }

    float3 tangent;
    float3 bitangent;
    BuildOrthonormalBasis(N, tangent, bitangent);

    // 時間に依存しないので、同じ点なら常に同じ結果になる
    float angle = rand3dTo1d(worldPos) * kAOTwoPI;
    float cosA = cos(angle);
    float sinA = sin(angle);

    float visible = 0.0f;
    for (uint i = 0; i < sampleCount; i++)
    {
        float2 hammersley = float2((i + 0.5f) / sampleCount, RadicalInverse(i));

        float diskRadius = sqrt(hammersley.x);
        float phi = kAOTwoPI * hammersley.y;
        float2 disk = float2(diskRadius * cos(phi), diskRadius * sin(phi));
        // 一定の並びのままだと縞になるので、点ごとに接空間を回す
        float2 rotated = float2(disk.x * cosA - disk.y * sinA, disk.x * sinA + disk.y * cosA);

        float3 dir = rotated.x * tangent + rotated.y * bitangent + sqrt(saturate(1.0f - hammersley.x)) * N;

        // 影と同じレイ。遮蔽されていれば0が返る
        visible += TraceShadowRay(worldPos, dir, N, radius);
    }

    return lerp(1.0f, visible / sampleCount, saturate(intensity));
}

#endif // RAYTRACED_AO_HLSLI
