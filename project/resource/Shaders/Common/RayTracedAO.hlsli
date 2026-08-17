#ifndef RAYTRACED_AO_HLSLI
#define RAYTRACED_AO_HLSLI

// インラインレイトレによるアンビエントオクルージョン。
// TLAS と RayQuery は影と共有するので、宣言はこちらに持たず RayTracedShadow.hlsli を使う
#include "RayTracedShadow.hlsli"
#include "Random.hlsli"
#include "Sampling.hlsli"

// AllLights::aoMode の値。C++ 側 (LightManager.h の RayTracedAOMode) と同じ値であること
static const uint kAOModeOff = 0;    // 計算しない
static const uint kAOModeInline = 1; // 前方描画のPSで直接飛ばす。デノイズできないので比較用
static const uint kAOModeScreen = 2; // 別パスで計算しデノイズしたものを画面空間で引く

/// <summary>遮蔽されていない割合を返す。1で開けている、0で完全に囲まれている</summary>
/// <remarks>cosine 重みなので重みが分布に入り、最後は当たった本数を割るだけでよい</remarks>
/// <param name="worldPos">シェーディングしている点のワールド座標</param>
/// <param name="N">面の法線</param>
/// <param name="sampleCount">飛ばすレイの本数</param>
/// <param name="radius">この距離までの遮蔽物だけを見る</param>
/// <param name="intensity">0で無効、1で素の遮蔽率</param>
/// <param name="rotationSeed">接空間を回す角度の種。同じ値なら同じ結果になる</param>
float TraceAO(float3 worldPos, float3 N, uint sampleCount, float radius, float intensity, float rotationSeed)
{
    if (sampleCount == 0 || radius <= 0.0f)
    {
        return 1.0f;
    }

    float3 tangent;
    float3 bitangent;
    BuildOrthonormalBasis(N, tangent, bitangent);

    float angle = rotationSeed * kSamplingTwoPI;
    float cosA = cos(angle);
    float sinA = sin(angle);

    float visible = 0.0f;
    for (uint i = 0; i < sampleCount; i++)
    {
        float2 hammersley = Hammersley(i, sampleCount);
        // 円板の半径が sqrt(x) なので、高さを sqrt(1-x) にすると cosine 重みの半球になる
        float2 disk = UniformDiskSample(hammersley, cosA, sinA);

        float3 dir = disk.x * tangent + disk.y * bitangent + sqrt(saturate(1.0f - hammersley.x)) * N;

        // 影と同じレイ。遮蔽されていれば0が返る
        visible += TraceShadowRay(worldPos, dir, N, radius);
    }

    return lerp(1.0f, visible / sampleCount, saturate(intensity));
}

#endif // RAYTRACED_AO_HLSLI
