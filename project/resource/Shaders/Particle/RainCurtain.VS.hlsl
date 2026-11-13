#include "Rain.hlsli"
#include "RainCurtain.hlsli"

// 画面を覆う三角形を層の距離の深度に置く

RainCurtainOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    RainCurtainOutput output = (RainCurtainOutput) 0;
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    float2 ndc = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float distance = RainCurtainDistance(instanceID);
    // 見ている向きの奥行きが distance の面の深度
    float depth = rainProjectionZScale + rainProjectionZOffset / distance;
    output.position = float4(ndc, depth, 1.0f);
    output.ndc = ndc;
    output.layer = instanceID;
    return output;
}
