#ifndef IBL_BAKE_HLSLI
#define IBL_BAKE_HLSLI

// IBL を焼く側(Shaders/IBL/*.CS.hlsl)だけが使う共通処理。
// 引く側(前方描画のPS)は Common/IBL.hlsli を見ること
#include "Sampling.hlsli"

/// <summary>キューブマップの面と面内のUVから、その texel が表す向きを求める</summary>
/// <remarks>D3D のキューブマップの並び(+X,-X,+Y,-Y,+Z,-Z)に合わせてあること</remarks>
/// <param name="face">面の番号(0〜5)。Texture2DArray のスライス番号と同じ</param>
/// <param name="uv">面内の位置。[0,1]</param>
float3 DirectionFromCubeFace(uint face, float2 uv)
{
    float2 st = uv * 2.0f - 1.0f;
    float3 dir;
    switch (face)
    {
        case 0: dir = float3(1.0f, -st.y, -st.x); break;  // +X
        case 1: dir = float3(-1.0f, -st.y, st.x); break;  // -X
        case 2: dir = float3(st.x, 1.0f, st.y); break;    // +Y
        case 3: dir = float3(st.x, -1.0f, -st.y); break;  // -Y
        case 4: dir = float3(st.x, -st.y, 1.0f); break;   // +Z
        default: dir = float3(-st.x, -st.y, -1.0f); break; // -Z
    }
    return normalize(dir);
}

/// <summary>GGX の分布に沿ってハーフベクトルを散らす</summary>
/// <remarks>一様に散らすと粗さが低いときサンプルが無駄になるので、分布そのものを確率密度に使う</remarks>
/// <param name="xi">[0,1)^2 の点</param>
/// <param name="N">畳み込む中心の向き</param>
float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = kSamplingTwoPI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));

    float3 h = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 tangent;
    float3 bitangent;
    BuildOrthonormalBasis(N, tangent, bitangent);

    return normalize(tangent * h.x + bitangent * h.y + N * h.z);
}

/// <summary>Smith の幾何減衰(IBL用)</summary>
/// <remarks>直接光用の G_SmithSchlick とは k の取り方が違う(直接光は (r+1)^2/8、IBLは a^2/2)</remarks>
float G_SmithIBL(float NdotV, float NdotL, float roughness)
{
    float a = roughness * roughness;
    float k = (a * a) / 2.0f;
    float gv = NdotV / (NdotV * (1.0f - k) + k);
    float gl = NdotL / (NdotL * (1.0f - k) + k);
    return gv * gl;
}

#endif // IBL_BAKE_HLSLI
