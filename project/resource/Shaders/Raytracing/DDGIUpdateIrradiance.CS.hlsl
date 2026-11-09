#include "../Common/DDGICommon.hlsli"

// プローブ1つ = 1グループ。レイの結果を八面体テクスチャへ cosine 重みで畳み込み、前の値と混ぜる

ConstantBuffer<DDGIVolume> gDDGIVolume : register(b0);
Texture2D<float4> gDDGIRayData : register(t0);
RWTexture2D<float4> gDDGIIrradianceOut : register(u0);

// 裏面に当たったレイがこの割合を超えたら埋まっているとみなす
static const float kBackfaceThreshold = 0.25f;
// これより大きく変わったら前の値を早く手放す (RTXGI の probeIrradianceThreshold)
static const float kIrradianceThreshold = 0.2f;
// これより大きく跳ねたら動く量を 1/4 に抑える (RTXGI の probeBrightnessThreshold)
static const float kBrightnessThreshold = 0.1f;

static const uint kTileSize = kDDGIIrradianceTexels + 2;

groupshared float3 gsRadiance[kDDGIMaxRaysPerProbe];
groupshared float3 gsDirection[kDDGIMaxRaysPerProbe];
groupshared float gsDistance[kDDGIMaxRaysPerProbe];
groupshared float4 gsResult[kDDGIIrradianceTexels][kDDGIIrradianceTexels];
groupshared uint gsBackfaceCount;

[numthreads(kDDGIIrradianceTexels, kDDGIIrradianceTexels, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID, uint threadIndex : SV_GroupIndex)
{
    DDGIVolume volume = gDDGIVolume;
    uint probeIndex = DDGIUpdatedProbeIndex(volume, groupID.x);
    int3 slot = DDGIIndexToSlot(volume, probeIndex);
    uint2 tileOrigin = DDGIIrradianceTile(volume, slot) * kTileSize;

    if (threadIndex == 0)
    {
        gsBackfaceCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    // レイの結果を共有メモリへ。1スレッドが2本ずつ
    const uint threadCount = kDDGIIrradianceTexels * kDDGIIrradianceTexels;
    for (uint r = threadIndex; r < volume.raysPerProbe; r += threadCount)
    {
        float4 data = gDDGIRayData[uint2(kDDGIFixedRays + r, DDGIRayRow(volume, probeIndex))];
        gsRadiance[r] = data.rgb;
        gsDistance[r] = data.a;
        gsDirection[r] = DDGIRayDirection(volume, r);
    }
    // 埋まっているかは向きの変わらないレイで数える
    if (threadIndex < kDDGIFixedRays && gDDGIRayData[uint2(threadIndex, DDGIRayRow(volume, probeIndex))].a < 0.0f)
    {
        InterlockedAdd(gsBackfaceCount, 1);
    }
    GroupMemoryBarrierWithGroupSync();

    float2 octUv = ((float2(threadID.xy) + 0.5f) / float(kDDGIIrradianceTexels)) * 2.0f - 1.0f;
    float3 texelDirection = DDGIOctDecode(octUv);

    float3 sum = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    for (uint i = 0; i < volume.raysPerProbe; i++)
    {
        if (gsDistance[i] < 0.0f)
        {
            continue;
        }
        float w = max(dot(texelDirection, gsDirection[i]), 0.0f);
        sum += gsRadiance[i] * w;
        weightSum += w;
    }
    float3 linearIrradiance = (weightSum > 1e-5f) ? sum / weightSum : float3(0.0f, 0.0f, 0.0f);
    float3 irradiance = pow(linearIrradiance, 1.0f / kDDGIIrradianceGamma);
    float isValid = (float(gsBackfaceCount) / float(kDDGIFixedRays) > kBackfaceThreshold) ? 0.0f : 1.0f;

    // 別のセルを受け持つことになったプローブは前の値を捨てる
    bool isReset = DDGIIsProbeReset(volume, slot);

    uint2 texel = tileOrigin + 1 + threadID.xy;
    float4 result = float4(irradiance, isValid);
    if (!isReset)
    {
        // 作り直さないときだけ前の値を読む
        float3 previous = gDDGIIrradianceOut[texel].rgb;
        if (volume.bakeComponent != kDDGIBakeNone)
        {
            // 焼くときはただの平均。符号化を戻して混ぜる
            float3 linearPrevious = pow(max(previous, 0.0f), kDDGIIrradianceGamma);
            result.rgb = pow(lerp(linearIrradiance, linearPrevious, volume.hysteresis), 1.0f / kDDGIIrradianceGamma);
        }
        else
        {
            float hysteresis = volume.hysteresis;
            float3 delta = irradiance - previous;
            if (max(max(abs(delta.r), abs(delta.g)), abs(delta.b)) > kIrradianceThreshold)
            {
                hysteresis = max(0.0f, hysteresis - 0.75f);
            }
            if (length(delta) > kBrightnessThreshold)
            {
                irradiance = previous + delta * 0.25f;
            }
            result.rgb = lerp(irradiance, previous, hysteresis);
        }
    }
    gDDGIIrradianceOut[texel] = result;
    gsResult[threadID.y][threadID.x] = result;
    GroupMemoryBarrierWithGroupSync();

    // 外周の縁に八面体の反対側の値を入れる
    const uint n = kDDGIIrradianceTexels;
    const uint borderCount = 4 * n + 4;
    for (uint b = threadIndex; b < borderCount; b += threadCount)
    {
        uint2 dst;
        uint2 src;
        if (b < n)
        {
            dst = uint2(1 + b, 0);
            src = uint2(n - 1 - b, 0);
        }
        else if (b < 2 * n)
        {
            uint k = b - n;
            dst = uint2(1 + k, n + 1);
            src = uint2(n - 1 - k, n - 1);
        }
        else if (b < 3 * n)
        {
            uint k = b - 2 * n;
            dst = uint2(0, 1 + k);
            src = uint2(0, n - 1 - k);
        }
        else if (b < 4 * n)
        {
            uint k = b - 3 * n;
            dst = uint2(n + 1, 1 + k);
            src = uint2(n - 1, n - 1 - k);
        }
        else
        {
            // 四隅は対角の内側
            uint k = b - 4 * n;
            dst = uint2((k & 1) ? n + 1 : 0, (k & 2) ? n + 1 : 0);
            src = uint2((k & 1) ? 0 : n - 1, (k & 2) ? 0 : n - 1);
        }
        gDDGIIrradianceOut[tileOrigin + dst] = gsResult[src.y][src.x];
    }
}
