#include "Skybox.hlsli"

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
    output.position = mul(input.position, gTransformationMatrix.WVP).xyww;
    output.texcoord = normalize(input.position.xyz);
    return output;
}