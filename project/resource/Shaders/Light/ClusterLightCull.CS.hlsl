#include "../Common/ClusteredLights.hlsli"

// クラスタごとに届くライトの一覧を作る (ClusteredLightPass)。1スレッドが1クラスタ
// クラスタはビュー空間の箱、ライトは届く距離の球として当てる

ConstantBuffer<ClusterParams> gClusterParams : register(b0);
StructuredBuffer<ClusteredLight> gClusteredLights : register(t0);
RWStructuredBuffer<uint> gClusterCountsOut : register(u0);
RWStructuredBuffer<uint> gClusterIndicesOut : register(u1);

// グループで手分けしてライトをビュー空間へ移してから全員で当てる
static const uint kLightBatch = kClusterCountX * kClusterCountY;
groupshared float4 gsLightSpheres[kLightBatch]; // xyz = ビュー空間の中心, w = 届く距離

[numthreads(kClusterCountX, kClusterCountY, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    ClusterParams params = gClusterParams;
    uint cluster = dispatchID.x + kClusterCountX * (dispatchID.y + kClusterCountY * dispatchID.z);

    // 段の手前と奥の距離 (対数)
    float depthRatio = params.clusterFar / params.clusterNear;
    float zNear = params.clusterNear * pow(depthRatio, float(dispatchID.z) / float(kClusterCountZ));
    float zFar = params.clusterNear * pow(depthRatio, float(dispatchID.z + 1) / float(kClusterCountZ));
    // いちばん奥の段は無限
    if (dispatchID.z == kClusterCountZ - 1)
    {
        zFar = 1e6f;
    }

    // 画素の y は下向き、NDC の y は上向き
    float2 ndcMin = float2(float(dispatchID.x) / kClusterCountX * 2.0f - 1.0f, 1.0f - float(dispatchID.y + 1) / kClusterCountY * 2.0f);
    float2 ndcMax = float2(float(dispatchID.x + 1) / kClusterCountX * 2.0f - 1.0f, 1.0f - float(dispatchID.y) / kClusterCountY * 2.0f);

    // 四隅を手前と奥の距離へ伸ばした8点を囲む箱
    float3 boxMin = float3(1e30f, 1e30f, zNear);
    float3 boxMax = float3(-1e30f, -1e30f, zFar);
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        float2 ndc = float2((i & 1) ? ndcMax.x : ndcMin.x, (i & 2) ? ndcMax.y : ndcMin.y);
        float2 perZ = ndc / params.projectionScale;
        float2 a = perZ * zNear;
        float2 b = perZ * zFar;
        boxMin.xy = min(boxMin.xy, min(a, b));
        boxMax.xy = max(boxMax.xy, max(a, b));
    }

    uint count = 0;
    // 同期を挟むので全スレッドが最後まで回る
    for (uint batchStart = 0; batchStart < params.lightCount; batchStart += kLightBatch)
    {
        uint loadIndex = batchStart + groupIndex;
        if (loadIndex < params.lightCount)
        {
            ClusteredLight light = gClusteredLights[loadIndex];
            gsLightSpheres[groupIndex] = float4(mul(float4(light.position, 1.0f), params.view).xyz, light.range + ClusteredLightExtent(light));
        }
        GroupMemoryBarrierWithGroupSync();

        uint batchCount = min(kLightBatch, params.lightCount - batchStart);
        for (uint i = 0; i < batchCount && count < kMaxLightsPerCluster; ++i)
        {
            float4 sphere = gsLightSpheres[i];
            // 球と箱の最も近い点までの距離
            float3 closest = clamp(sphere.xyz, boxMin, boxMax);
            float3 d = sphere.xyz - closest;
            if (dot(d, d) <= sphere.w * sphere.w)
            {
                gClusterIndicesOut[cluster * kMaxLightsPerCluster + count] = batchStart + i;
                ++count;
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }
    gClusterCountsOut[cluster] = count;
}
