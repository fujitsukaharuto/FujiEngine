#include "../Common/IBLBake.hlsli"
#include "../Common/PBR.hlsli"

// 反射プローブのキューブを粗さごとに GGX で畳んで配列へ書く。PrefilterEnv と違い a も畳む

TextureCube<float4> gProbeCapture : register(t0);
RWTexture2DArray<float4> gProbePrefilteredOut : register(u0);
SamplerState gSampler : register(s0);

// ReflectionProbePass.h の PrefilterParam と並びが一致していること
cbuffer ProbePrefilterParam : register(b0)
{
    float roughness;
    uint sampleCount;
    uint sourceSize;  // 焼いたキューブの1辺
    uint sliceOffset; // 書き先の最初の面 (プローブの番号 × 6)
    uint outputSize;  // 書くミップの1辺
    uint3 prefilterPad;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (dispatchID.x >= outputSize || dispatchID.y >= outputSize)
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / float(outputSize);
    float3 normal = DirectionFromCubeFace(dispatchID.z, uv);
    // N = V = R の割り切りは PrefilterEnv と同じ
    float3 view = normal;
    float texelSolidAngle = 4.0f * kPI / (6.0f * float(sourceSize) * float(sourceSize));

    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
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

        float NdotH = saturate(dot(normal, halfVec));
        float VdotH = saturate(dot(view, halfVec));
        float pdf = D_GGX(NdotH, roughness) * NdotH / max(4.0f * VdotH, 1e-4f) + 1e-4f;
        float sampleSolidAngle = 1.0f / (float(sampleCount) * pdf);
        float mip = (roughness <= 0.0f) ? 0.0f : 0.5f * log2(sampleSolidAngle / texelSolidAngle);

        color += gProbeCapture.SampleLevel(gSampler, light, max(mip, 0.0f)) * NdotL;
        totalWeight += NdotL;
    }

    gProbePrefilteredOut[uint3(dispatchID.xy, dispatchID.z + sliceOffset)] = color / max(totalWeight, 1e-4f);
}
