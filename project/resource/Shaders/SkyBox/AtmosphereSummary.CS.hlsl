#include "AtmosphereModel.hlsli"
#include "../Common/Sampling.hlsli"

// 空から CPU 側で使う値 (フォグの色・半球アンビエント・空気遠近の係数) を書き出す

Texture2D<float4> gSkyViewLut : register(t0);
Texture2D<float4> gTransmittanceLut : register(t1);
Texture2D<float4> gMultiScatterLut : register(t2);
// 雲まで描き込んだ空。半球アンビエントはここから取る
TextureCube<float4> gSkyCube : register(t3);
RWStructuredBuffer<float4> gSummaryOut : register(u0);
SamplerState gSampler : register(s0);

// 並びは SkyAtmosphere.h の SummaryIndex と一致していること
static const uint kSummaryHorizon = 0;        // +X, -X, +Z, -Z の地平線の色
static const uint kSummaryAmbientUp = 4;      // 上向きの面に届く空の光
static const uint kSummaryAmbientDown = 5;    // 下向きの面に届く地面からの照り返し
static const uint kSummarySunTransmittance = 6;
static const uint kSummaryMultiScattering = 7;

static const uint kThreadCount = 64;
static const float kAmbientCubeMip = 3.0f;
static const uint kSamplesPerThread = 4;

groupshared float3 gsUp[kThreadCount];
groupshared float3 gsDown[kThreadCount];

float3 SampleSky(float3 direction)
{
    return gSkyViewLut.SampleLevel(gSampler, SkyViewDirectionToUv(direction, AtmosphereHorizonZenith()), 0.0f).rgb;
}

[numthreads(kThreadCount, 1, 1)]
void main(uint3 threadID : SV_GroupThreadID)
{
    const uint sampleCount = kThreadCount * kSamplesPerThread;
    float3 up = float3(0.0f, 0.0f, 0.0f);
    float3 down = float3(0.0f, 0.0f, 0.0f);
    for (uint s = 0; s < kSamplesPerThread; s++)
    {
        // cosine 重みで散らすと平均が「放射照度 / PI」になる
        float2 xi = Hammersley(threadID.x * kSamplesPerThread + s, sampleCount);
        float2 disk = UniformDiskSample(xi, 1.0f, 0.0f);
        float h = sqrt(saturate(1.0f - xi.x));
        // ぼかしたミップから引く
        up += gSkyCube.SampleLevel(gSampler, normalize(float3(disk.x, h, disk.y)), kAmbientCubeMip).rgb;
        down += gSkyCube.SampleLevel(gSampler, normalize(float3(disk.x, -h, disk.y)), kAmbientCubeMip).rgb;
    }
    gsUp[threadID.x] = up;
    gsDown[threadID.x] = down;
    GroupMemoryBarrierWithGroupSync();

    if (threadID.x != 0)
    {
        return;
    }

    float3 sumUp = float3(0.0f, 0.0f, 0.0f);
    float3 sumDown = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < kThreadCount; i++)
    {
        sumUp += gsUp[i];
        sumDown += gsDown[i];
    }
    gSummaryOut[kSummaryAmbientUp] = float4(sumUp / float(sampleCount), 1.0f);
    gSummaryOut[kSummaryAmbientDown] = float4(sumDown / float(sampleCount), 1.0f);

    // 地平線のわずかに上の帯の平均
    const float3 kHorizonDirections[4] = { float3(1, 0, 0), float3(-1, 0, 0), float3(0, 0, 1), float3(0, 0, -1) };
    for (uint side = 0; side < 4; side++)
    {
        float3 color = float3(0.0f, 0.0f, 0.0f);
        for (uint e = 0; e < 3; e++)
        {
            float elevation = 0.02f + 0.03f * float(e);
            color += SampleSky(normalize(kHorizonDirections[side] + float3(0.0f, elevation, 0.0f)));
        }
        gSummaryOut[kSummaryHorizon + side] = float4(color / 3.0f, 1.0f);
    }

    float3 origin = float3(0.0f, AtmosphereViewHeight(), 0.0f);
    gSummaryOut[kSummarySunTransmittance] = float4(SunTransmittance(gTransmittanceLut, gSampler, origin, sunDirection), 1.0f);
    gSummaryOut[kSummaryMultiScattering] = float4(MultiScattering(gMultiScatterLut, gSampler, origin, sunDirection), 1.0f);
}
