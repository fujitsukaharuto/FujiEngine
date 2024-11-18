#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInversTransPose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);



VertxShaderOutput main(VertexShaderInput input)
{
    VertxShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInversTransPose));
    
    //reflectionModel
    output.WorldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    
    return output;
}