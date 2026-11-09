#include "../Object/Object3d.hlsli"

// 画面のプローブを置き、半球へレイを飛ばして球面調和関数の係数へ畳む。1グループ = プローブ1つ、1スレッド = レイ1本

Texture2D<float4> gTextures[] : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b0);
#include "../Common/RayHitLighting.hlsli"
#include "../Common/ScreenProbeCommon.hlsli"

RWTexture2D<float4> gProbeSHOut : register(u0);       // 横に R,G,B の3枚
RWTexture2D<float4> gProbePositionOut : register(u1); // xyz=位置, w=使えるか
RWTexture2D<float4> gProbeNormalOut : register(u2);   // xyz=法線, w=カメラからの距離

// 1本の明るさの上限。白い点のちらつきを抑える
static const float kMaxRayRadiance = 16.0f;

groupshared float4 gsSH[3][kScreenProbeRays];

[numthreads(kScreenProbeRays, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint threadIndex : SV_GroupIndex)
{
    uint2 probe = groupID.xy;

    // 置く画素はタイルの中でフレームごとにずらす。空ならタイルの中心
    uint2 tileOrigin = probe * tileSize;
    uint2 pixel = tileOrigin + min(uint2(ScreenProbeHash2(probe, frameIndex) * float(tileSize)), tileSize - 1);
    pixel = min(pixel, screenSize - 1);
    float3 normal;
    float depth;
    bool isValid = LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth);
    if (!isValid)
    {
        pixel = min(tileOrigin + tileSize / 2, screenSize - 1);
        isValid = LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth);
    }

    float3 position = float3(0.0f, 0.0f, 0.0f);
    float viewDistance = 0.0f;
    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float3 direction = float3(0.0f, 1.0f, 0.0f);
    if (isValid)
    {
        position = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
        viewDistance = distance(position, cameraPos);

        // 半球を 8x8 に割った向きを R2 列でフレームごとにずらす
        float2 jitter = frac(ScreenProbeHash2(probe + 7u, 0u) + float(frameIndex & 1023u) * float2(0.7548776662f, 0.5698402910f));
        float2 u = (float2(threadIndex % kScreenProbeRaysSqrt, threadIndex / kScreenProbeRaysSqrt) + jitter)
                   / float(kScreenProbeRaysSqrt);
        float cosTheta = u.x;
        float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
        float phi = kSamplingTwoPI * u.y;
        float3 tangent;
        float3 bitangent;
        BuildOrthonormalBasis(normal, tangent, bitangent);
        direction = normalize(tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + normal * cosTheta);

        RayDesc ray;
        ray.Origin = position + normal * ReconstructedRayBias(viewDistance);
        ray.Direction = direction;
        ray.TMin = 0.0f;
        ray.TMax = maxRayDistance;

        RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
        q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
        q.Proceed();

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            radiance = RayMissRadiance(direction);
        }
        else if (q.CommittedTriangleFrontFace())
        {
            // 裏面に当たったレイは暗いまま
            RayHitSurface surface = LoadRayHitSurface(q.CommittedInstanceID() + q.CommittedGeometryIndex(),
                                                      q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics(),
                                                      q.CommittedObjectToWorld3x4(), ray.Origin, direction, q.CommittedRayT());
            radiance = ShadeRayHit(surface, direction, (gLights.renderFlags & kRenderFlagGI) != 0);
        }
        float peak = max(radiance.r, max(radiance.g, radiance.b));
        if (peak > kMaxRayRadiance)
        {
            radiance *= kMaxRayRadiance / peak;
        }
    }

    // 一様な半球の確率密度は 1/2π。係数は「明るさ x 基底 / 密度」の平均
    float4 basis = SHBasis(direction) * (kSamplingTwoPI / float(kScreenProbeRays));
    gsSH[0][threadIndex] = radiance.r * basis;
    gsSH[1][threadIndex] = radiance.g * basis;
    gsSH[2][threadIndex] = radiance.b * basis;
    GroupMemoryBarrierWithGroupSync();

    [unroll]
    for (uint stride = kScreenProbeRays / 2; stride > 0; stride >>= 1)
    {
        if (threadIndex < stride)
        {
            gsSH[0][threadIndex] += gsSH[0][threadIndex + stride];
            gsSH[1][threadIndex] += gsSH[1][threadIndex + stride];
            gsSH[2][threadIndex] += gsSH[2][threadIndex + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (threadIndex == 0)
    {
        gProbeSHOut[uint2(probe.x * 3 + 0, probe.y)] = gsSH[0][0];
        gProbeSHOut[uint2(probe.x * 3 + 1, probe.y)] = gsSH[1][0];
        gProbeSHOut[uint2(probe.x * 3 + 2, probe.y)] = gsSH[2][0];
        gProbePositionOut[probe] = float4(position, isValid ? 1.0f : 0.0f);
        gProbeNormalOut[probe] = float4(normal, viewDistance);
    }
}
