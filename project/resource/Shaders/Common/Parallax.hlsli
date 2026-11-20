#ifndef PARALLAX_HLSLI
#define PARALLAX_HLSLI

// 視差マッピング (Parallax Occlusion Mapping / Tatarchuk 2006、UE の ParallaxOcclusionMapping と同じ考え方)。
// 高さのテクスチャ (白が高い) の中を視線に沿って段を刻んで進み、最初に面の下へ潜った段と1つ前の段の間を
// 直線でつないで交わる UV を返す。面そのものは動かさない (影と深度は元の面のまま)

// 段の数。正面から見ると少なく、寝かせるほど多い
static const float kParallaxMinSteps = 8.0f;
static const float kParallaxMaxSteps = 32.0f;

/// <summary>視差でずらした UV</summary>
/// <param name="uv">ずらす前の UV</param>
/// <param name="worldPosition">画素のワールド座標 (接空間を画面の微分から作る)</param>
/// <param name="normal">面の向き (法線マップ前)</param>
/// <param name="toEye">画素からカメラへの単位ベクトル</param>
/// <param name="heightScale">高さ1の段差が UV で何ぶんずれて見えるか (UE の Height Ratio)</param>
float2 ParallaxOcclusionUV(Texture2D<float4> heightMap, SamplerState smp, float2 uv, float3 worldPosition, float3 normal,
                           float3 toEye, float heightScale)
{
    // UV の u と v が伸びる向き (Schüler 2013 の余接の組)。頂点の接線に頼らないので、裏返して貼った UV でも合う
    float3 dp1 = ddx(worldPosition);
    float3 dp2 = ddy(worldPosition);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);
    float3 dp2perp = cross(dp2, normal);
    float3 dp1perp = cross(normal, dp1);
    float3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 bitangent = dp2perp * duv1.y + dp1perp * duv2.y;
    float frameScale = rsqrt(max(max(dot(tangent, tangent), dot(bitangent, bitangent)), 1e-20f));
    tangent *= frameScale;
    bitangent *= frameScale;

    // 視線を接空間へ。寝かせすぎた向きは z を下限で止めて、ずれが無限に伸びないようにする
    float3 viewTS = float3(dot(toEye, tangent), dot(toEye, bitangent), dot(toEye, normal));
    float viewZ = max(viewTS.z, 0.05f);
    float stepCount = lerp(kParallaxMaxSteps, kParallaxMinSteps, saturate(viewTS.z));
    float layerDepth = 1.0f / stepCount;
    float2 deltaUV = viewTS.xy / viewZ * (heightScale / stepCount);

    // 段の途中でミップが飛ばないよう、元の UV の微分で引く
    float2 currentUV = uv;
    float currentDepth = 0.0f;
    float surfaceDepth = 1.0f - heightMap.SampleGrad(smp, currentUV, duv1, duv2).r;
    [loop]
    for (int i = 0; i < int(kParallaxMaxSteps) && currentDepth < surfaceDepth; i++)
    {
        currentUV -= deltaUV;
        currentDepth += layerDepth;
        surfaceDepth = 1.0f - heightMap.SampleGrad(smp, currentUV, duv1, duv2).r;
    }

    // いちばん上 (白) なら動かない
    if (currentDepth <= 0.0f)
    {
        return uv;
    }
    // 潜った段と1つ前の段の間で、視線と面が交わる所
    float2 previousUV = currentUV + deltaUV;
    float after = surfaceDepth - currentDepth;
    float before = (1.0f - heightMap.SampleGrad(smp, previousUV, duv1, duv2).r) - (currentDepth - layerDepth);
    float weight = after / min(after - before, -1e-5f);
    return lerp(currentUV, previousUV, saturate(weight));
}

#endif // PARALLAX_HLSLI
