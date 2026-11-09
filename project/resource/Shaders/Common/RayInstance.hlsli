#ifndef RAY_INSTANCE_HLSLI
#define RAY_INSTANCE_HLSLI

// レイが当たった物の見た目と形。反射に映す色と、抜きのある物の影に使う

// RayInstance::flags のビット。C++ 側 (RaytracingScene.h の RayInstanceFlag) と同じ値であること
static const uint kRayInstanceHasGeometry = 1u << 0; // 頂点とインデックスを引ける
static const uint kRayInstanceHasNormal = 1u << 1;   // 頂点の法線が使える
static const uint kRayInstanceCutout = 1u << 2;      // テクスチャに抜きがある
// 抜きのしきい値 (Material の alphaRef) を 8bit で置く位置
static const uint kRayInstanceAlphaRefShift = 8;

// C++ 側 (RaytracingScene.h の RayInstanceData) と並びが一致していること
struct RayInstance
{
    float3 color; // マテリアルの色
    uint textureIndex;
    float3 emissive;
    uint vertexBuffer; // gRayVertices[] の番号
    uint indexBuffer; // gRayIndices[] の番号
    uint flags;
    float2 uvScale;
    float2 uvOffset;
    float roughness; // 映り込みを付けていないマテリアルは1
    float metallic;
    uint emissiveTextureIndex; // 発光に掛けるテクスチャ。無ければ色のテクスチャ
    float opacity; // 1未満は半透明
    uint2 padEmissive;
};

// メッシュごとに1つ。番号は「インスタンスの番号 + メッシュの番号」
StructuredBuffer<RayInstance> gRayInstances : register(t1, space2);

// C++ 側 (Mesh.h の VertexData) と並びが一致していること
struct RayVertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
};

// どちらも gTextures[] と同じヒープを指す
StructuredBuffer<RayVertex> gRayVertices[] : register(t0, space4);
Buffer<uint> gRayIndices[] : register(t0, space5);

#ifdef RAY_CUTOUT_SHADOW
// gTextures[] と同じヒープ。gTextures[] を持たない CS 用
Texture2D<float4> gRayTextures[] : register(t0, space8);

/// <summary>抜きのある三角形の当たった所が抜けていないか</summary>
/// <remarks>アルファが alphaRef 以下なら抜け。最も細かいミップを直接読む</remarks>
/// <param name="instanceIndex">InstanceID + GeometryIndex</param>
bool RayCutoutIsSolid(uint instanceIndex, uint primitiveIndex, float2 barycentrics)
{
    RayInstance instance = gRayInstances[instanceIndex];
    if ((instance.flags & kRayInstanceCutout) == 0 || (instance.flags & kRayInstanceHasGeometry) == 0)
    {
        return true;
    }

    uint first = primitiveIndex * 3;
    Buffer<uint> indices = gRayIndices[NonUniformResourceIndex(instance.indexBuffer)];
    StructuredBuffer<RayVertex> vertices = gRayVertices[NonUniformResourceIndex(instance.vertexBuffer)];
    float2 uv0 = vertices[indices[first + 0]].texcoord;
    float2 uv1 = vertices[indices[first + 1]].texcoord;
    float2 uv2 = vertices[indices[first + 2]].texcoord;
    float2 uv = uv0 * (1.0f - barycentrics.x - barycentrics.y) + uv1 * barycentrics.x + uv2 * barycentrics.y;
    // Material::SetUVScale と同じ順で、繰り返す
    uv = frac((uv + instance.uvOffset) * instance.uvScale);

    Texture2D<float4> texture = gRayTextures[NonUniformResourceIndex(instance.textureIndex)];
    uint2 size;
    texture.GetDimensions(size.x, size.y);
    uint2 texel = min(uint2(uv * float2(size)), size - 1);
    float alpha = texture.Load(int3(texel, 0)).a;
    float alphaRef = float((instance.flags >> kRayInstanceAlphaRefShift) & 0xFFu) / 255.0f;
    return alpha > alphaRef;
}
#endif // RAY_CUTOUT_SHADOW

#endif // RAY_INSTANCE_HLSLI
