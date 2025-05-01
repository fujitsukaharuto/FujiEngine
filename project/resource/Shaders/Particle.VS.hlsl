#include "Particle.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4 color;
    float2 uvTrans;
    float2 uvScale;
};
StructuredBuffer<TransformationMatrix> gTransformationMatries : register(t0);



VertxShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertxShaderOutput output;
    output.position = mul(input.position, gTransformationMatries[instanceId].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatries[instanceId].World));
    output.color = gTransformationMatries[instanceId].color;
    output.uvTrans = gTransformationMatries[instanceId].uvTrans;
    output.uvScale = gTransformationMatries[instanceId].uvScale;
    return output;
}