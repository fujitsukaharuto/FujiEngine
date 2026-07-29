#include "CSParticle.hlsli"

struct DrawArguments
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

RWStructuredBuffer<DrawIndexedArgs> gDrawArgs : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    gDrawArgs[0].IndexCountPerInstance = 6;
    gDrawArgs[0].InstanceCount = 0;
    gDrawArgs[0].StartIndexLocation = 0;
    gDrawArgs[0].BaseVertexLocation = 0;
    gDrawArgs[0].StartInstanceLocation = 0;
}