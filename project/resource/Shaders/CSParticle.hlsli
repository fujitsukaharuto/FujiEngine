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

static const float3 kGravity = float3(0.0f, -8.0f, 0.0f);
static const uint kMaxParticles = 10485760;
struct Particle
{
    float3 translate;

    float3 scale;
    float3 startScale;

    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;

    float3 prevTranslate;
    uint isRandomMove;
    uint isTrailEmit;
    uint isGravity;
};

struct Particle_Translate
{
    float3 translate;
    float3 prevTranslate;
};

struct Particle_Scale
{
    float3 scale;
    float3 startScale;
};

struct Particle_Time
{
    float lifetime;
    float currentTime;
};

struct Particle_Velocity
{
    float3 velocity;
};

struct Particle_color
{
    float4 color;
};

struct Particle_Flags
{
    uint isRandomMove;
    uint isTrailEmit;
    uint isGravity;
};