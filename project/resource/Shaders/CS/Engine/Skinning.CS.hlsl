struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Vertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

struct VertexInfluence
{
    float4 weight;
    int4 index;
};

struct SkinningInformation
{
    uint numVertices;
};

struct MeshSection
{
    uint vertexOffset; // gInputVertices[] の先頭からのオフセット
    uint vertexCount; // このメッシュの頂点数
    uint matrixPaletteOffset; // gMatrixPallette[] のオフセット
    uint materialIndex; // 描画で使う
};

StructuredBuffer<Well> gMatrixPallette : register(t0);
StructuredBuffer<Vertex> gInputVertices : register(t1);
StructuredBuffer<VertexInfluence> gInfluences : register(t2);
StructuredBuffer<MeshSection> gMeshSections : register(t3);
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);
ConstantBuffer<SkinningInformation> gSkinningInformation : register(b0);

cbuffer MeshCB : register(b1)
{
    uint meshIndex;
};

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    MeshSection section = gMeshSections[meshIndex];
    
    uint vertexIndex = DTid.x;
    if (vertexIndex >= gSkinningInformation.numVertices || vertexIndex >= section.vertexCount)
        return;

    uint globalIndex = vertexIndex;
    uint globalIndex2 = vertexIndex+section.vertexOffset;

    Vertex input = gInputVertices[globalIndex];
    VertexInfluence infl = gInfluences[globalIndex2];

    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNorm = float3(0, 0, 0);

    skinnedPos = mul(input.position, gMatrixPallette[infl.index.x].skeletonSpaceMatrix) * infl.weight.x;
    skinnedPos += mul(input.position, gMatrixPallette[infl.index.y].skeletonSpaceMatrix) * infl.weight.y;
    skinnedPos += mul(input.position, gMatrixPallette[infl.index.z].skeletonSpaceMatrix) * infl.weight.z;
    skinnedPos += mul(input.position, gMatrixPallette[infl.index.w].skeletonSpaceMatrix) * infl.weight.w;
    skinnedPos.w = 1.0f;

    skinnedNorm = mul(input.normal, (float3x3) gMatrixPallette[infl.index.x].skeletonSpaceInverseTransposeMatrix) * infl.weight.x;
    skinnedNorm += mul(input.normal, (float3x3) gMatrixPallette[infl.index.y].skeletonSpaceInverseTransposeMatrix) * infl.weight.y;
    skinnedNorm += mul(input.normal, (float3x3) gMatrixPallette[infl.index.z].skeletonSpaceInverseTransposeMatrix) * infl.weight.z;
    skinnedNorm += mul(input.normal, (float3x3) gMatrixPallette[infl.index.w].skeletonSpaceInverseTransposeMatrix) * infl.weight.w;
    skinnedNorm = normalize(skinnedNorm);

    Vertex output;
    output.position = skinnedPos;
    output.normal = skinnedNorm;
    output.texcoord = input.texcoord;

    gOutputVertices[globalIndex] = output;
    
}