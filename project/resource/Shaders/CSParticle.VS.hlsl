#include "CSParticle.hlsli"

StructuredBuffer<Particle_Translate> gParticles_Trans : register(t0);
StructuredBuffer<Particle_Scale> gParticles_Scale : register(t1);
StructuredBuffer<Particle_Color> gParticles_Color : register(t2);
struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};
ConstantBuffer<PerView> gPerView : register(b0);


VertxShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertxShaderOutput output;
    float4x4 worldMatrix = gPerView.billboardMatrix;
    worldMatrix[0] *= gParticles_Scale[instanceId].scale.x;
    worldMatrix[1] *= gParticles_Scale[instanceId].scale.y;
    worldMatrix[2] *= gParticles_Scale[instanceId].scale.z;
    worldMatrix[3].xyz = gParticles_Trans[instanceId].translate;
    output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
    output.texcoord = input.texcoord;
    output.color = gParticles_Color[instanceId].color;
    return output;
}