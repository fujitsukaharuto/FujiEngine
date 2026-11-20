#include "AtmosphereModel.hlsli"

// カメラから見た全方位の空の色。毎フレーム作る

Texture2D<float4> gTransmittanceLut : register(t0);
Texture2D<float4> gMultiScatterLut : register(t1);
RWTexture2D<float4> gSkyViewOut : register(u0);
SamplerState gSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(float2(dispatchID.xy) >= kSkyViewLutSize))
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / kSkyViewLutSize;
    float3 direction = SkyViewUvToDirection(uv, AtmosphereHorizonZenith());
    float3 origin = float3(0.0f, AtmosphereViewHeight(), 0.0f);
    // 大気の外から大気を通らない向きは宇宙
    if (!MoveToTopAtmosphere(origin, direction))
    {
        gSkyViewOut[dispatchID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float groundDistance = RaySphere(origin, direction, bottomRadius);
    float distance = (groundDistance > 0.0f) ? groundDistance : RaySphere(origin, direction, topRadius);
    distance = max(distance, 0.0f);

    // 地平線ほど歩数を増やす
    uint steps = uint(lerp(16.0f, 32.0f, saturate(distance / 150.0f)));
    float dt = distance / float(steps);

    float cosTheta = dot(direction, sunDirection);
    float phaseRayleigh = RayleighPhase(cosTheta);
    float phaseMie = MiePhase(miePhaseG, cosTheta);

    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float3 luminance = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < steps; i++)
    {
        float3 position = origin + direction * ((float(i) + 0.5f) * dt);
        AtmosphereMedium medium = SampleMedium(position);
        float3 extinction = max(medium.extinction, 1e-6f);
        float3 stepTransmittance = exp(-extinction * dt);

        float3 sunTransmittance = SunTransmittance(gTransmittanceLut, gSampler, position, sunDirection);
        float3 multiScattering = MultiScattering(gMultiScatterLut, gSampler, position, sunDirection);
        float3 source = sunTransmittance * (medium.rayleigh * phaseRayleigh + medium.mie * phaseMie)
                        + multiScattering * medium.scattering;

        luminance += throughput * source * (1.0f - stepTransmittance) / extinction;
        throughput *= stepTransmittance;
    }

    // 地平線より下は照らされた地面の色
    if (groundDistance > 0.0f)
    {
        float3 position = origin + direction * groundDistance;
        float3 normal = normalize(position);
        float3 sunTransmittance = SunTransmittance(gTransmittanceLut, gSampler, position, sunDirection);
        luminance += throughput * sunTransmittance * saturate(dot(normal, sunDirection)) * groundAlbedo / kAtmospherePI;
    }

    gSkyViewOut[dispatchID.xy] = float4(luminance * sunIlluminance, 1.0f);
}
