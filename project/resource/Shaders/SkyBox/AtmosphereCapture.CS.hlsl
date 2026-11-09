#include "AtmosphereModel.hlsli"
#include "../Common/IBLBake.hlsli"

// SkyView の数表をキューブマップへ写す。太陽の円盤は入れず、雲は入れる

Texture2D<float4> gSkyViewLut : register(t0);
Texture2D<float4> gTransmittanceLut : register(t1);
RWTexture2DArray<float4> gSkyCubeOut : register(u0);
SamplerState gSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    uint faceCount;
    gSkyCubeOut.GetDimensions(width, height, faceCount);
    if (dispatchID.x >= width || dispatchID.y >= height)
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / float2(width, height);
    float3 direction = DirectionFromCubeFace(dispatchID.z, uv);
    float3 sky = gSkyViewLut.SampleLevel(gSampler, SkyViewDirectionToUv(direction), 0.0f).rgb;

    if (cloudCoverage > 0.0f)
    {
        float3 cloudPosition = float3(0.0f, bottomRadius + cameraHeight + kCloudHeight * 0.001f, 0.0f);
        float3 sunAtCloud = sunIlluminance * SunTransmittance(gTransmittanceLut, gSampler, cloudPosition, sunDirection);
        float3 zenith = gSkyViewLut.SampleLevel(gSampler, SkyViewDirectionToUv(float3(0.0f, 1.0f, 0.0f)), 0.0f).rgb;
        // 原点から見た雲にする
        float4 cloud = CloudLayer(direction, sunDirection, sunAtCloud, zenith, sky, cloudCoverage, cloudTime, float2(0.0f, 0.0f));
        sky = lerp(sky, cloud.rgb, cloud.a);
    }
    gSkyCubeOut[dispatchID] = float4(sky, 1.0f);
}
