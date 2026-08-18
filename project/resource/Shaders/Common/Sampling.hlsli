#ifndef SAMPLING_HLSLI
#define SAMPLING_HLSLI

// レイを半球や円錐へ散らすときの共通処理。AO(半球)とソフトシャドウ(円錐)が同じものを使う。
// 散らし方が2箇所に分かれると、片方だけ縞が出るといった追いにくい差になるのでここへ集約する

static const float kSamplingPI = 3.14159265359f;
static const float kSamplingTwoPI = 6.28318530718f;

/// <summary>単位ベクトルから接空間の基底を作る(Duff et al. の分岐なし版)</summary>
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

/// <summary>count 本のうち i 番目の Hammersley 点</summary>
float2 Hammersley(uint i, uint count)
{
    return float2((i + 0.5f) / count, RadicalInverse(i));
}

/// <summary>単位円板に一様に散らした点</summary>
/// <remarks>
/// 半径に sqrt を掛けるのは面積で一様にするため。x を半径に使うので、
/// 呼び出し側は 1-x の平方根をそのまま cosine 重みの高さとして使える
/// </remarks>
/// <param name="xi">[0,1)^2 の点</param>
/// <param name="cosA">並び全体を回す角度。一定の並びのままだと縞になる</param>
/// <param name="sinA">同上</param>
float2 UniformDiskSample(float2 xi, float cosA, float sinA)
{
    float radius = sqrt(xi.x);
    float phi = kSamplingTwoPI * xi.y;
    float2 disk = float2(radius * cos(phi), radius * sin(phi));

    return float2(disk.x * cosA - disk.y * sinA, disk.x * sinA + disk.y * cosA);
}

#endif // SAMPLING_HLSLI
