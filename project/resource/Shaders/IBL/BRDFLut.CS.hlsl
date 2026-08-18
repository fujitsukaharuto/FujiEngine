#include "../Common/IBLBake.hlsli"

// split-sum 近似の第2項を焼く。環境にもマテリアルにも依存しない純粋な数表なので、
// 一度焼けば内容は永久に同じ(環境マップを差し替えても焼き直し不要)。
// x軸 = N・V、y軸 = 粗さ。出力の x を F0 に掛け、y を足すと鏡面の反射率になる

RWTexture2D<float2> gBRDFLutOut : register(u0);

static const uint kSampleCount = 1024;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    gBRDFLutOut.GetDimensions(width, height);
    if (dispatchID.x >= width || dispatchID.y >= height)
    {
        return;
    }

    // 端の texel が 0 や 1 ちょうどにならないよう半 texel ずらす。
    // N・V = 0 は視線が面と平行で分母が消える
    float NdotV = max((float(dispatchID.x) + 0.5f) / float(width), 1e-3f);
    float roughness = (float(dispatchID.y) + 0.5f) / float(height);

    // 法線を +Z に固定して、視線をその周りに置く(結果は N・V にしか依存しない)
    float3 normal = float3(0.0f, 0.0f, 1.0f);
    float3 view = float3(sqrt(saturate(1.0f - NdotV * NdotV)), 0.0f, NdotV);

    float scale = 0.0f;
    float bias = 0.0f;

    for (uint i = 0; i < kSampleCount; i++)
    {
        float2 xi = Hammersley(i, kSampleCount);
        float3 halfVec = ImportanceSampleGGX(xi, normal, roughness);
        float3 light = normalize(2.0f * dot(view, halfVec) * halfVec - view);

        float NdotL = light.z;
        if (NdotL <= 0.0f)
        {
            continue;
        }

        float NdotH = saturate(halfVec.z);
        float VdotH = saturate(dot(view, halfVec));

        float g = G_SmithIBL(NdotV, NdotL, roughness);
        float gVis = g * VdotH / max(NdotH * NdotV, 1e-4f);
        // フレネルを F0 の1次式に分解し、係数(scale)と定数項(bias)を別々に積む
        float fc = pow(saturate(1.0f - VdotH), 5.0f);

        scale += (1.0f - fc) * gVis;
        bias += fc * gVis;
    }

    gBRDFLutOut[dispatchID.xy] = float2(scale, bias) / float(kSampleCount);
}
