#include "../Common/IBLBake.hlsli"
#include "../Common/PBR.hlsli"

// split-sum 近似の第2項を焼く。環境にもマテリアルにも依存しないので焼き直し不要。
// x軸 = N・V、y軸 = 粗さ。出力の x を F0 に掛け、y を足すと鏡面の反射率になる。
// z は布の光沢 (sheen) の反射率 E(N・V, 粗さ)。下の層をどれだけ暗くするかと、環境光の光沢に使う (Filament の DFG の3つ目と同じ)

RWTexture2D<float4> gBRDFLutOut : register(u0);

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

    // 布の光沢。半球を一様に散らした H で積む (pdf は 1/2π、L への変換で 4 V・H)
    float sheen = 0.0f;
    for (uint j = 0; j < kSampleCount; j++)
    {
        float2 xi = Hammersley(j, kSampleCount);
        float cosTheta = 1.0f - xi.x;
        float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
        float phi = 2.0f * kPI * xi.y;
        float3 halfVec = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
        float3 light = 2.0f * dot(view, halfVec) * halfVec - view;
        float NdotL = saturate(light.z);
        if (NdotL <= 0.0f)
        {
            continue;
        }
        float VdotH = saturate(dot(view, halfVec));
        sheen += D_Charlie(saturate(halfVec.z), roughness) * V_Neubelt(NdotV, NdotL) * NdotL * VdotH;
    }
    sheen *= 4.0f * 2.0f * kPI / float(kSampleCount);

    gBRDFLutOut[dispatchID.xy] = float4(scale / float(kSampleCount), bias / float(kSampleCount), sheen, 1.0f);
}
