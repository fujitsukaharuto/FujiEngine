#include "../Common/IBLBake.hlsli"

// 環境マップを法線方向の放射照度に畳み込んで小さいキューブマップへ焼く。
// 出力は cosine 重みの平均放射輝度(= 放射照度 / PI)

TextureCube<float4> gSourceEnv : register(t0);
RWTexture2DArray<float4> gIrradianceOut : register(u0);
SamplerState gSampler : register(s0);

cbuffer IrradianceParam : register(b0)
{
    // 元の環境マップの1辺。サンプルごとのミップ選択に使う
    uint sourceSize;
    uint sampleCount;
    uint2 irradiancePad;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    uint faceCount;
    gIrradianceOut.GetDimensions(width, height, faceCount);
    if (dispatchID.x >= width || dispatchID.y >= height)
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / float2(width, height);
    float3 normal = DirectionFromCubeFace(dispatchID.z, uv);

    float3 tangent;
    float3 bitangent;
    BuildOrthonormalBasis(normal, tangent, bitangent);

    // 半球へ N 本散らすと1本が受け持つ立体角はおよそ 2PI/N。見合ったミップまで落とさないと、
    // HDRIの太陽のように小さくて明るい所の拾い方がサンプルごとに変わって斑になる
    float texelSolidAngle = 4.0f * kSamplingPI / (6.0f * float(sourceSize) * float(sourceSize));
    float sampleSolidAngle = kSamplingTwoPI / float(sampleCount);
    float sourceMip = max(0.5f * log2(sampleSolidAngle / texelSolidAngle), 0.0f);

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < sampleCount; i++)
    {
        float2 xi = Hammersley(i, sampleCount);
        // cosine 重みの半球サンプル。重みと確率密度が打ち消し合うので、
        // 合計をサンプル数で割るだけでよい(余弦も PI も掛けない)
        float2 disk = UniformDiskSample(xi, 1.0f, 0.0f);
        float height01 = sqrt(saturate(1.0f - xi.x));
        float3 dir = tangent * disk.x + bitangent * disk.y + normal * height01;

        irradiance += gSourceEnv.SampleLevel(gSampler, normalize(dir), sourceMip).rgb;
    }

    gIrradianceOut[dispatchID] = float4(irradiance / float(sampleCount), 1.0f);
}
