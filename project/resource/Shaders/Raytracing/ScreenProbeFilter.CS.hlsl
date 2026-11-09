#include "../Common/ScreenProbeCommon.hlsli"

// 画面のプローブを、同じ面に乗っている隣どうしで混ぜる

Texture2D<float4> gProbeSH : register(t4, space7);
Texture2D<float4> gProbePosition : register(t5, space7);
Texture2D<float4> gProbeNormal : register(t6, space7);
RWTexture2D<float4> gProbeSHFilteredOut : register(u0);

// これ以上自分の面から離れた隣は混ぜない (距離に対する割合)
static const float kPlaneTolerance = 0.03f;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    int2 probe = int2(dispatchID.xy);
    if (any(dispatchID.xy >= probeCount))
    {
        return;
    }

    float4 center = gProbePosition.Load(int3(probe, 0));
    if (center.w <= 0.0f)
    {
        [unroll]
        for (uint c = 0; c < 3; c++)
        {
            gProbeSHFilteredOut[uint2(probe.x * 3 + c, probe.y)] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        return;
    }
    float4 centerNormal = gProbeNormal.Load(int3(probe, 0));
    float tolerance = max(kPlaneTolerance * centerNormal.w, 0.02f);

    float4 sum[3] = { float4(0.0f, 0.0f, 0.0f, 0.0f), float4(0.0f, 0.0f, 0.0f, 0.0f), float4(0.0f, 0.0f, 0.0f, 0.0f) };
    float weightSum = 0.0f;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            int2 neighbor = probe + int2(x, y);
            if (any(neighbor < 0) || any(neighbor >= int2(probeCount)))
            {
                continue;
            }
            float4 position = gProbePosition.Load(int3(neighbor, 0));
            if (position.w <= 0.0f)
            {
                continue;
            }
            float3 normal = gProbeNormal.Load(int3(neighbor, 0)).xyz;
            float planeDistance = abs(dot(centerNormal.xyz, position.xyz - center.xyz));
            float weight = exp(-planeDistance / tolerance) * pow(saturate(dot(centerNormal.xyz, normal)), 8.0f);
            // 自分を少し重くする
            weight *= (x == 0 && y == 0) ? 2.0f : 1.0f;
            [unroll]
            for (uint c = 0; c < 3; c++)
            {
                sum[c] += gProbeSH.Load(int3(neighbor.x * 3 + c, neighbor.y, 0)) * weight;
            }
            weightSum += weight;
        }
    }

    [unroll]
    for (uint c = 0; c < 3; c++)
    {
        gProbeSHFilteredOut[uint2(probe.x * 3 + c, probe.y)] = sum[c] / weightSum;
    }
}
