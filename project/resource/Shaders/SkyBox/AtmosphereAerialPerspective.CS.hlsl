#include "AtmosphereModel.hlsli"
#include "../Common/AerialPerspective.hlsli"

// 空気遠近の3Dの表。視野の縦横1マスごとにレイを進め、奥行きの段ごとに書く

Texture2D<float4> gTransmittanceLut : register(t0);
Texture2D<float4> gMultiScatterLut : register(t1);
RWTexture3D<float4> gAerialScatterOut : register(u0);
RWTexture3D<float4> gAerialTransmittanceOut : register(u1);
SamplerState gSampler : register(s0);

// 段1つの間を何歩で進むか
static const uint kStepsPerSlice = 2;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(float2(dispatchID.xy) >= kAerialLutSize.xy))
    {
        return;
    }

    // このマスの中心を通る視線
    float2 uv = (float2(dispatchID.xy) + 0.5f) / kAerialLutSize.xy;
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float4 nearPoint = mul(float4(ndc, 0.0f, 1.0f), gAerialPerspective.invViewProj);
    float4 farPoint = mul(float4(ndc, 1.0f, 1.0f), gAerialPerspective.invViewProj);
    float3 direction = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);

    float3 origin = float3(0.0f, AtmosphereViewHeight(), 0.0f);
    float cosTheta = dot(direction, sunDirection);
    float phaseRayleigh = RayleighPhase(cosTheta);
    float phaseMie = MiePhase(miePhaseG, cosTheta);

    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float3 luminance = float3(0.0f, 0.0f, 0.0f);
    float t = 0.0f;
    for (uint slice = 0; slice < uint(kAerialLutSize.z); slice++)
    {
        // 段の中心の距離。引く側と同じく平方根で並べる
        float w = (float(slice) + 0.5f) / kAerialLutSize.z;
        float sliceEnd = w * w * gAerialPerspective.maxDistanceKm;
        float dt = (sliceEnd - t) / float(kStepsPerSlice);
        for (uint i = 0; i < kStepsPerSlice; i++)
        {
            float3 position = origin + direction * (t + (float(i) + 0.5f) * dt);
            // 地面の下へ潜るレイは地表の空気のまま進める
            float radius = length(position);
            position *= max(radius, bottomRadius + 1e-4f) / radius;

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
        t = sliceEnd;

        uint3 texel = uint3(dispatchID.xy, slice);
        gAerialScatterOut[texel] = float4(luminance * gAerialPerspective.sunIlluminance, 1.0f);
        gAerialTransmittanceOut[texel] = float4(throughput, 1.0f);
    }
}
