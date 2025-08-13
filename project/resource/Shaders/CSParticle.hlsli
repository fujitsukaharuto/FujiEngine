struct VertxShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

static const uint kMaxParticles = 1048576;
struct Particle
{
    float3 translate;
    
    float3 scale;
    float3 startScale;
    
    
    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;
};