#ifndef FLOWING_WATER_HLSLI
#define FLOWING_WATER_HLSLI

// 流れる水 (置物の flowWater)。側溝の水など、UV の u を m 単位にしたモデル向け。
// u が小さくなる向き (流れ着く先) へ動くさざ波の法線を立てる。接線は UV と位置の画面の微分から求めるので、モデルに接線は要らない

// さざ波の数
static const uint kFlowWaveCount = 4;
// 波ごとの (波長 m, 流れからの角度, 傾きの強さ, 流れの速さの倍率)
static const float4 kFlowWaves[kFlowWaveCount] =
{
    float4(0.11f, 0.35f, 0.16f, 1.00f),
    float4(0.07f, -0.50f, 0.12f, 1.15f),
    float4(0.045f, 0.15f, 0.09f, 0.90f),
    float4(0.17f, -0.20f, 0.10f, 0.80f),
};

/// <param name="speed">流れの速さ (m/s)。0なら何もしない。負なら u の大きくなる向きへ流れる</param>
/// <param name="uv">モデルの UV (u が m 単位)</param>
/// <param name="time">秒</param>
void ApplyFlowingWater(float speed, float2 uv, float3 worldPos, float time, inout float3 normal)
{
    if (speed == 0.0f)
    {
        return;
    }
    // u が増える向きのワールドの向き
    float2 duvdx = ddx(uv);
    float2 duvdy = ddy(uv);
    float3 dpdx = ddx(worldPos);
    float3 dpdy = ddy(worldPos);
    float det = duvdx.x * duvdy.y - duvdx.y * duvdy.x;
    if (abs(det) < 1e-12f)
    {
        return;
    }
    float3 alongU = (dpdx * duvdy.y - dpdy * duvdx.y) / det;
    float3 flow = -alongU * sign(speed);
    flow -= normal * dot(flow, normal);
    float flowLength = length(flow);
    if (flowLength < 1e-6f)
    {
        return;
    }
    flow /= flowLength;
    float3 across = cross(normal, flow);
    float2 p = float2(dot(worldPos, flow), dot(worldPos, across));
    // 1画素の大きさ。波長がこれに近い波は消す (遠くでちらつかない)
    float footprint = max(length(dpdx), length(dpdy));
    float absSpeed = abs(speed);

    float2 slope = float2(0.0f, 0.0f);
    [unroll]
    for (uint i = 0; i < kFlowWaveCount; i++)
    {
        float4 w = kFlowWaves[i];
        float k = 6.2831853f / w.x;
        float2 dir = float2(cos(w.y), sin(w.y));
        float phase = k * (dot(p, dir) - absSpeed * w.w * time) + float(i) * 1.7f;
        float fade = saturate(w.x / (footprint * 6.0f) - 0.5f);
        slope += dir * (w.z * cos(phase) * fade);
    }
    normal = normalize(normal - flow * slope.x - across * slope.y);
}

#endif // FLOWING_WATER_HLSLI
