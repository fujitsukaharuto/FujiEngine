struct VertxShaderOutput
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};