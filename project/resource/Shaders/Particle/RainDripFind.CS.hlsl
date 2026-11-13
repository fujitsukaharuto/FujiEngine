#define RAIN_LIGHTS
#include "Rain.hlsli"

// 雨だれが垂れる場所を探す。カメラのまわりの格子の1マスに1か所。
// 雨を受ける面 (空から下へのレイが最初に当たる面) の縁で、その先が大きく下がっている所から垂らす。
// 庇・看板の上・電線などに当たる。結果は取っておき、マスが入れ替わったときと、少しずつの探し直しで書き換える

RaytracingAccelerationStructure gSceneTLAS : register(t0, space2);
RWStructuredBuffer<RainDripSite> gRainDripSitesOut : register(u0);

// RainRenderer.h の kDripGridSize_ と同じ値であること
static const uint kDripGridSize = 128;
// 1フレームに探し直すマスの割合 (1 / この数)
static const uint kDripRefreshInterval = 64;
// 動かない不透明だけに当てる。RayTracedShadow.hlsli の kRayMaskStaticOpaque と同じ値
static const uint kDripRayMask = 0x04;
// これより低い面からは垂らさない (地面・歩道)
static const float kDripMinTop = 0.35f;
// 縁の先がこれだけ下がっていれば縁とみなす
static const float kDripMinDrop = 0.35f;

/// <summary>上から下へのレイが最初に当たる高さ。当たらなければ false</summary>
bool TraceDown(float2 xz, float fromY, float maxDistance, out float height)
{
    RayDesc ray;
    ray.Origin = float3(xz.x, fromY, xz.y);
    ray.Direction = float3(0.0f, -1.0f, 0.0f);
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kDripRayMask, ray);
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            q.CommitNonOpaqueTriangleHit();
        }
    }
    height = fromY - q.CommittedRayT();
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

/// <summary>下から上へのレイが最初に当たる高さ。当たらなければ false</summary>
bool TraceUp(float2 xz, float fromY, float maxDistance, out float height)
{
    RayDesc ray;
    ray.Origin = float3(xz.x, fromY, xz.y);
    ray.Direction = float3(0.0f, 1.0f, 0.0f);
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(gSceneTLAS, RAY_FLAG_NONE, kDripRayMask, ray);
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            q.CommitNonOpaqueTriangleHit();
        }
    }
    height = fromY + q.CommittedRayT();
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

/// <summary>1マス分を探す。垂れないマスは state = 1</summary>
RainDripSite FindDripSite(int2 cell)
{
    RainDripSite site = (RainDripSite) 0;
    site.cell = cell;
    site.state = 1u;

    uint cellHash = uint(cell.x) * 73856093u ^ uint(cell.y) * 19349663u;
    float3 random = RainHash3(cellHash);
    site.seed = random.z;
    float2 p = (float2(cell) + 0.15f + random.xy * 0.7f) * dripCellSize;
    float skyY = rainCameraPosition.y + 300.0f;

    float top;
    if (!TraceDown(p, skyY, 600.0f, top) || top < kDripMinTop)
    {
        return site;
    }

    // まわり4方向で、いちばん下がっている向きを縁の外とする
    float angle = random.z * 1.5707963f;
    float2 outward = float2(0.0f, 0.0f);
    float below = top;
    [unroll]
    for (uint k = 0; k < 4; k++)
    {
        float a = angle + 1.5707963f * float(k);
        float2 dir = float2(cos(a), sin(a));
        float h;
        if (!TraceDown(p + dir * dripCellSize, skyY, 600.0f, h))
        {
            h = -1000.0f;
        }
        if (h < below)
        {
            below = h;
            outward = dir;
        }
    }
    if (top - below < kDripMinDrop)
    {
        return site;
    }

    // 縁の位置を二分探索で詰める (まだ上の面がある側を残す)
    float covered = 0.0f;
    float open = 1.0f;
    [unroll]
    for (uint s = 0; s < 4; s++)
    {
        float m = (covered + open) * 0.5f;
        float h;
        bool hit = TraceDown(p + outward * (dripCellSize * m), skyY, 600.0f, h);
        if (hit && h > top - 0.15f)
        {
            covered = m;
        }
        else
        {
            open = m;
        }
    }
    float2 edge = p + outward * (dripCellSize * covered);

    // 垂れ始めるのは上の面の裏側。下から上へのレイで探す
    float floorY = max(below, -0.5f) + 0.03f;
    float underside;
    if (!TraceUp(edge, floorY, max(top - floorY + 0.05f, 0.01f), underside) || underside > top + 0.01f)
    {
        underside = top - 0.03f;
    }
    float land;
    if (!TraceDown(edge, underside - 0.01f, dripMaxFall + 1.0f, land))
    {
        return site;
    }
    float fall = underside - land;
    if (fall < 0.25f || fall > dripMaxFall)
    {
        return site;
    }
    site.site = float4(edge.x, underside, edge.y, land);
    site.state = 2u;
    return site;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= kDripGridSize || id.y >= kDripGridSize)
    {
        return;
    }
    uint slot = id.y * kDripGridSize + id.x;
    // この枠が受け持つのは、今の格子の中で番号を格子の大きさで割った余りが同じマス
    int2 offset = (int2(id.xy) - dripGridBase) % int(kDripGridSize);
    offset = offset + int2(offset < 0) * int(kDripGridSize);
    int2 cell = dripGridBase + offset;

    RainDripSite current = gRainDripSitesOut[slot];
    bool isStale = current.state == 0u || any(current.cell != cell) ||
                   ((slot + dripFrame) % kDripRefreshInterval) == 0u;
    if (!isStale)
    {
        return;
    }
    gRainDripSitesOut[slot] = FindDripSite(cell);
}
