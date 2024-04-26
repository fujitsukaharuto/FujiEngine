#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);



VertxShaderOutput main(VertexShaderInput input)
{
    VertxShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    return output;
}