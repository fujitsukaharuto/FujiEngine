#include "../Common/IBLBake.hlsli"
#include "../Common/PBR.hlsli"

// 環境マップを粗さごとに GGX で畳み込んで、ミップ列として焼く。
// ミップ番号がそのまま粗さに対応するので、引く側は roughness からミップを選ぶだけでよい。
// 1ミップにつき1ディスパッチ。UAV はそのミップだけを指す

TextureCube<float4> gSourceEnv : register(t0);
RWTexture2DArray<float4> gPrefilteredOut : register(u0);
SamplerState gSampler : register(s0);

cbuffer PrefilterParam : register(b0)
{
    float roughness;
    uint sampleCount;
    // 元の環境マップの1辺。サンプルごとのミップ選択に使う
    uint sourceSize;
    uint prefilterPad;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    uint faceCount;
    gPrefilteredOut.GetDimensions(width, height, faceCount);
    if (dispatchID.x >= width || dispatchID.y >= height)
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / float2(width, height);
    float3 normal = DirectionFromCubeFace(dispatchID.z, uv);

    // 見る向きが分からないので N = V = R と置く。粗い面ほど本来の形からずれるが、
    // その代わり視線に依存しない1枚のマップで済む(業界の標準的な割り切り)
    float3 view = normal;

    // 1 texel が覆う立体角。サンプルの密度がこれより粗いときは元のミップを下げて拾う
    float texelSolidAngle = 4.0f * kPI / (6.0f * float(sourceSize) * float(sourceSize));

    float3 color = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    for (uint i = 0; i < sampleCount; i++)
    {
        float2 xi = Hammersley(i, sampleCount);
        float3 halfVec = ImportanceSampleGGX(xi, normal, roughness);
        float3 light = normalize(2.0f * dot(view, halfVec) * halfVec - view);

        float NdotL = dot(normal, light);
        if (NdotL <= 0.0f)
        {
            continue;
        }

        // サンプルが疎な所で元の解像度を引くと、明るい1点が斑点として残る。
        // そのサンプルが受け持つ立体角に見合ったミップまで落として拾う
        float NdotH = saturate(dot(normal, halfVec));
        float VdotH = saturate(dot(view, halfVec));
        float pdf = D_GGX(NdotH, roughness) * NdotH / max(4.0f * VdotH, 1e-4f) + 1e-4f;
        float sampleSolidAngle = 1.0f / (float(sampleCount) * pdf);
        float mip = (roughness <= 0.0f) ? 0.0f : 0.5f * log2(sampleSolidAngle / texelSolidAngle);

        color += gSourceEnv.SampleLevel(gSampler, light, max(mip, 0.0f)).rgb * NdotL;
        totalWeight += NdotL;
    }

    gPrefilteredOut[dispatchID] = float4(color / max(totalWeight, 1e-4f), 1.0f);
}
