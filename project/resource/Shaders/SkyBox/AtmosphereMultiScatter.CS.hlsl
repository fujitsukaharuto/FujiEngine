#include "AtmosphereModel.hlsli"

// 多重散乱の数表。2回目の散乱を全方位で求め、等比級数で足し切る (Hillaire 2020)

Texture2D<float4> gTransmittanceLut : register(t0);
RWTexture2D<float4> gMultiScatterOut : register(u0);
SamplerState gSampler : register(s0);

// 全方位を 8x8 の向きで代表させる
static const uint kDirectionSqrt = 8;
static const uint kMultiScatterSteps = 20;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(float2(dispatchID.xy) >= kMultiScatterLutSize))
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / kMultiScatterLutSize;
    float sunZenithCos = uv.x * 2.0f - 1.0f;
    // 地表からわずかに浮かせる
    float height = bottomRadius + max(uv.y * (topRadius - bottomRadius), 0.01f);

    float3 origin = float3(0.0f, height, 0.0f);
    float3 sunDir = float3(sqrt(saturate(1.0f - sunZenithCos * sunZenithCos)), sunZenithCos, 0.0f);
    const float isotropicPhase = 1.0f / (4.0f * kAtmospherePI);

    float3 secondOrder = float3(0.0f, 0.0f, 0.0f);
    float3 transfer = float3(0.0f, 0.0f, 0.0f);
    for (uint d = 0; d < kDirectionSqrt * kDirectionSqrt; d++)
    {
        float2 cell = (float2(d % kDirectionSqrt, d / kDirectionSqrt) + 0.5f) / float(kDirectionSqrt);
        float phi = cell.x * 2.0f * kAtmospherePI;
        float cosTheta = 1.0f - 2.0f * cell.y;
        float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
        float3 direction = float3(sinTheta * cos(phi), cosTheta, sinTheta * sin(phi));

        float groundDistance = RaySphere(origin, direction, bottomRadius);
        float distance = (groundDistance > 0.0f) ? groundDistance : RaySphere(origin, direction, topRadius);
        float dt = max(distance, 0.0f) / float(kMultiScatterSteps);

        float3 throughput = float3(1.0f, 1.0f, 1.0f);
        float3 luminance = float3(0.0f, 0.0f, 0.0f);
        float3 scatteredAsOne = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < kMultiScatterSteps; i++)
        {
            float3 position = origin + direction * ((float(i) + 0.5f) * dt);
            AtmosphereMedium medium = SampleMedium(position);
            float3 extinction = max(medium.extinction, 1e-6f);
            float3 stepTransmittance = exp(-extinction * dt);
            // 区間の中の透過率の変化を解析的に積分する
            float3 integral = (1.0f - stepTransmittance) / extinction;

            float3 sunTransmittance = SunTransmittance(gTransmittanceLut, gSampler, position, sunDir);
            luminance += throughput * medium.scattering * sunTransmittance * isotropicPhase * integral;
            scatteredAsOne += throughput * medium.scattering * integral;
            throughput *= stepTransmittance;
        }

        // 地面で跳ね返った太陽の光も足す
        if (groundDistance > 0.0f)
        {
            float3 position = origin + direction * groundDistance;
            float3 normal = normalize(position);
            float3 sunTransmittance = SunTransmittance(gTransmittanceLut, gSampler, position, sunDir);
            luminance += throughput * sunTransmittance * saturate(dot(normal, sunDir)) * groundAlbedo / kAtmospherePI;
        }

        secondOrder += luminance;
        transfer += scatteredAsOne;
    }

    // 全方位の平均。位相関数の 1/4PI と立体角 4PI が打ち消し合う
    float directionCount = float(kDirectionSqrt * kDirectionSqrt);
    secondOrder /= directionCount;
    transfer /= directionCount;

    // 1 + r + r^2 + ... = 1/(1-r)
    float3 multiScattering = secondOrder / max(1.0f - transfer, 1e-3f);
    gMultiScatterOut[dispatchID.xy] = float4(multiScattering, 1.0f);
}
