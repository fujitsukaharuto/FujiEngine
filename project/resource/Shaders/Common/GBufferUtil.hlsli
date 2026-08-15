#ifndef GBUFFER_UTIL_HLSLI
#define GBUFFER_UTIL_HLSLI

// G-Buffer(深度+法線)を読む側の共通処理。AOのトレース/時間蓄積/空間フィルタが使う

/// <summary>非線形深度と画素位置からワールド座標へ戻す</summary>
/// <param name="pixel">画素の整数座標</param>
/// <param name="screenSize">画面の画素数</param>
/// <param name="depth">深度バッファの値(0が手前)</param>
/// <param name="invViewProj">ViewProjectionの逆行列。前フレームの点を戻すなら前フレームのもの</param>
float3 ReconstructWorldPos(uint2 pixel, uint2 screenSize, float depth, float4x4 invViewProj)
{
    float2 uv = (float2(pixel) + 0.5f) / float2(screenSize);
    // UVのVは下向き、NDCのYは上向きなので符号を反転する
    float4 clip = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth, 1.0f);
    float4 world = mul(clip, invViewProj);
    return world.xyz / world.w;
}

/// <summary>その画素に面が描かれているか。描かれていれば正規化した法線を返す</summary>
/// <remarks>プリパスは法線を0クリアしてから描くので、長さ0が「何も描かれていない」の目印になる</remarks>
bool LoadSurface(Texture2D<float4> normalTex, Texture2D<float> depthTex, int2 pixel,
                 out float3 normal, out float depth)
{
    int3 coord = int3(pixel, 0);

    normal = normalTex.Load(coord).xyz;
    depth = depthTex.Load(coord).r;

    float lengthSq = dot(normal, normal);
    if (lengthSq < 1e-6f || depth >= 1.0f)
    {
        normal = float3(0.0f, 0.0f, 0.0f);
        return false;
    }

    normal *= rsqrt(lengthSq);
    return true;
}

#endif // GBUFFER_UTIL_HLSLI
