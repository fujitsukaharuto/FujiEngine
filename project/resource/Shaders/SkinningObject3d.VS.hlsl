#include "SkinningObject3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInversTransPose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

//struct Well {
//    float4x4 skeletonSpaceMatrix;
//    float4x4 skeletonSpaceInverseTransposeMatrix;
//};
//StructuredBuffer<Well> gMatrixPallette : register(t0);

//struct Skinned{
//    float4 position;
//    float3 normal;
//};

//Skinned Skinning(VertexShaderInput input) {
//    Skinned skinned;
//    // 位置の変換
//    skinned.position = mul(input.position, gMatrixPallette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
//    skinned.position += mul(input.position, gMatrixPallette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
//    skinned.position += mul(input.position, gMatrixPallette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
//    skinned.position += mul(input.position, gMatrixPallette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
//    skinned.position.w = 1.0f;
    
//    // 法線の変換
//    skinned.normal = mul(input.normal, (float3x3) gMatrixPallette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
//    skinned.normal += mul(input.normal, (float3x3) gMatrixPallette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
//    skinned.normal += mul(input.normal, (float3x3) gMatrixPallette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
//    skinned.normal += mul(input.normal, (float3x3) gMatrixPallette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
//    skinned.normal = normalize(skinned.normal);
    
//    return skinned;
//}


VertxShaderOutput main(VertexShaderInput input) {
    //VertxShaderOutput output;
    //Skinned skinned = Skinning(input);

    //output.position = mul(skinned.position, gTransformationMatrix.WVP);
    //output.WorldPosition = mul(skinned.position, gTransformationMatrix.World).xyz;
    //output.texcoord = input.texcoord;
    //output.normal = normalize(mul(skinned.normal, (float3x3) gTransformationMatrix.WorldInversTransPose));

    //return output;
    VertxShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInversTransPose));
    
    //reflectionModel
    output.WorldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    
    return output;
}