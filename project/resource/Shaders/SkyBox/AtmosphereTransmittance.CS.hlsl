#include "AtmosphereModel.hlsli"

// 透過率の数表。ある高さと向きから大気の上端まで光が通り抜ける割合

RWTexture2D<float4> gTransmittanceOut : register(u0);

// 上ほど指数的に薄くなるだけなのでこの歩数で足りる
static const uint kTransmittanceSteps = 40;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(float2(dispatchID.xy) >= kTransmittanceLutSize))
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / kTransmittanceLutSize;
    float viewHeight;
    float viewZenithCos;
    TransmittanceUvToParams(uv, viewHeight, viewZenithCos);

    float3 origin = float3(0.0f, viewHeight, 0.0f);
    float3 direction = float3(sqrt(saturate(1.0f - viewZenithCos * viewZenithCos)), viewZenithCos, 0.0f);
    float distance = max(RaySphere(origin, direction, topRadius), 0.0f);

    float dt = distance / float(kTransmittanceSteps);
    float3 opticalDepth = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < kTransmittanceSteps; i++)
    {
        float3 position = origin + direction * ((float(i) + 0.5f) * dt);
        opticalDepth += SampleMedium(position).extinction * dt;
    }

    gTransmittanceOut[dispatchID.xy] = float4(exp(-opticalDepth), 1.0f);
}
