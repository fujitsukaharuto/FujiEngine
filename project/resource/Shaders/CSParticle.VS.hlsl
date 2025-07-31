#include "CSParticle.hlsli"

StructuredBuffer<Particle> gParticles : register(t0);
struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};
ConstantBuffer<PerView> gPerView : register(b0);


VertxShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertxShaderOutput output;
    Particle particle = gParticles[instanceId];
    float4x4 worldMatrix = gPerView.billboardMatrix;
    worldMatrix[0] *= particle.scale.x;
    worldMatrix[1] *= particle.scale.y;
    worldMatrix[2] *= particle.scale.z;
    worldMatrix[3].xyz = particle.translate;
    output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
    output.texcoord = input.texcoord;
    output.color = particle.color;
    return output;
}