#ifndef REFLECTION_PROBE_HLSLI
#define REFLECTION_PROBE_HLSLI

// 反射プローブ (ReflectionProbePass) を引く側。焼いたのは物の光 (rgb) と当たった割合 (a) で、空は焼いていない

// space0〜10 は使用済みなので space11
TextureCubeArray<float4> gReflectionProbes : register(t0, space11);

// ReflectionProbePass.h の kMaxProbes と一致していること
static const uint kReflectionProbeMaxCount = 8;
// ReflectionProbe::flags のビット
static const uint kReflectionProbeBoxProjection = 1u << 0; // 映った物を箱の壁にあるとみなして向きを直す
static const uint kReflectionProbeSliceShift = 8;          // 上位はキューブの配列の番号

// ReflectionProbePass.h の ProbeData と並びが一致していること
struct ReflectionProbe
{
    float3 capturePosition; // 焼いた位置 (箱の中心)
    float blendDistance;    // 箱の端からこの距離で外へ溶かす
    float3 boxMin;
    float intensity;
    float3 boxMax;
    uint flags;
};

// ReflectionProbePass.h の ProbeParams と並びが一致していること
struct ReflectionProbeParams
{
    ReflectionProbe probes[kReflectionProbeMaxCount]; // 大きい箱から順。後ろほど優先
    uint probeCount; // 0 なら無い
    float3 probePad;
};
// b4〜b7 は使用済みなので b8
ConstantBuffer<ReflectionProbeParams> gReflectionProbeParams : register(b8);

/// <summary>箱の中にいる度合い。端から blendDistance の内側で1になる</summary>
float ReflectionProbeInfluence(ReflectionProbe probe, float3 position)
{
    float3 inside = min(position - probe.boxMin, probe.boxMax - position);
    float distanceToEdge = min(inside.x, min(inside.y, inside.z));
    return saturate(distanceToEdge / max(probe.blendDistance, 1e-3f));
}

/// <summary>キューブを引く向き (box projection)</summary>
float3 ReflectionProbeDirection(ReflectionProbe probe, float3 position, float3 R)
{
    if ((probe.flags & kReflectionProbeBoxProjection) == 0)
    {
        return R;
    }
    // 0 で割らないよう寄せる
    float3 safeR = (step(0.0f, R) * 2.0f - 1.0f) * max(abs(R), 1e-5f);
    float3 toMax = (probe.boxMax - position) / safeR;
    float3 toMin = (probe.boxMin - position) / safeR;
    float3 farthest = max(toMax, toMin);
    float t = max(min(farthest.x, min(farthest.y, farthest.z)), 0.0f);
    return position + R * t - probe.capturePosition;
}

/// <summary>その点の映り込みを、置いた反射プローブから引く</summary>
/// <remarks>箱が重なれば後ろの番号を優先する (BakedGI.hlsli と同じ)</remarks>
/// <param name="R">反射の向き</param>
/// <param name="mip">粗さから決めたミップ</param>
/// <param name="influence">どれかの箱の中にいる度合い(0〜1)</param>
/// <returns>rgb=物の光、a=当たった割合。どちらも箱の度合いで割ってある</returns>
float4 SampleReflectionProbes(float3 position, float3 R, float mip, SamplerState smp, out float influence)
{
    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    influence = 0.0f;
    float remaining = 1.0f;
    [loop]
    for (int i = int(gReflectionProbeParams.probeCount) - 1; i >= 0 && remaining > 0.0f; i--)
    {
        ReflectionProbe probe = gReflectionProbeParams.probes[i];
        float weight = ReflectionProbeInfluence(probe, position);
        if (weight <= 0.0f)
        {
            continue;
        }
        float3 direction = ReflectionProbeDirection(probe, position, R);
        float slice = float(probe.flags >> kReflectionProbeSliceShift);
        float4 probeColor = gReflectionProbes.SampleLevel(smp, float4(direction, slice), mip);
        probeColor.rgb *= probe.intensity;
        result += probeColor * (remaining * weight);
        influence += remaining * weight;
        remaining *= 1.0f - weight;
    }
    if (influence > 0.0f)
    {
        result /= influence;
    }
    return result;
}

#endif // REFLECTION_PROBE_HLSLI
