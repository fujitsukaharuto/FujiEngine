struct VSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = input.position;
    output.texcoord = input.texcoord;
    return output;
}