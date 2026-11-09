#ifndef DDGI_COMMON_HLSLI
#define DDGI_COMMON_HLSLI

// プローブによる間接光 (DDGI / RTXGI と同じ方式) の定数と計算

// DDGIPass.h の DDGIVolumeData と並びが一致していること
struct DDGIVolume
{
    int3 baseCell;          // 格子の一番手前の隅のセル
    float spacing;          // プローブの間隔
    int3 previousBaseCell;  // 前のフレームの baseCell
    uint raysPerProbe;
    int3 probeCounts;       // 各軸のプローブの数
    float hysteresis;       // 前のフレームの値をどれだけ残すか
    float4 rayRotation[3];  // レイの向きを回す行列 (3行)
    float normalBias;       // 引くときに法線方向へ浮かせる量
    float viewBias;         // 同、視線方向
    float maxRayDistance;
    uint resetAll;          // 0以外なら全プローブを作り直す
    float2 irradianceAtlasSize;
    float2 distanceAtlasSize;
    uint probeStride;       // probeStride 個おきに更新する
    uint probeOffset;       // 何番目から始めるか
    uint firstProbe;        // レイの結果でこの格子が始まる行
    uint firstTileRow;      // アトラスでこの格子が始まるタイルの行
    uint bakeComponent;     // kDDGIBake*。0 なら毎フレーム更新する格子
    uint irradianceRowOffset; // 放射照度のアトラスだけ、タイルの行に足す量
    uint bakeFlags;         // kDDGIBakeFlag* の組み合わせ
    float bakePad;
    float3 bakeToSun;       // 太陽の成分を焼くときの太陽への向き
    float bakeSunPad;
};

// DDGIVolume::bakeComponent の値。BakedGIPass.h の BakeComponent と同じ値であること
static const uint kDDGIBakeNone = 0;    // 毎フレームの格子 (DDGIPass)
static const uint kDDGIBakeSkyUp = 1;   // 半球アンビエントの上の色の割合ぶん
static const uint kDDGIBakeSkyDown = 2; // 同、下の色
static const uint kDDGIBakeSun = 3;     // 強さ1の白い太陽
// DDGIVolume::bakeFlags のビット。BakedGIPass.cpp と同じ値であること
static const uint kDDGIBakeFlagUseOffsets = 1u << 0; // 再配置でずらした位置からレイを飛ばす

// 格子の数。0 がいちばん粗い。DDGIPass.h の kVolumeCount_ と一致していること
static const uint kDDGIVolumeCount = 3;

/// <summary>ディスパッチの番号から、このフレームに更新するプローブの番号</summary>
uint DDGIUpdatedProbeIndex(DDGIVolume volume, uint dispatchIndex)
{
    return dispatchIndex * volume.probeStride + volume.probeOffset;
}

// 八面体テクスチャの内側の大きさ。DDGIPass.h と一致していること
static const uint kDDGIIrradianceTexels = 8;
static const uint kDDGIDistanceTexels = 16;
static const uint kDDGIMaxRaysPerProbe = 128;
// 向きを回さないレイの本数 (RTXGI の probeNumFixedRays)。埋まり判定と再配置に使い、放射照度には混ぜない
static const uint kDDGIFixedRays = 32;
// 放射照度の符号化のガンマ (RTXGI の probeIrradianceEncodingGamma)
static const float kDDGIIrradianceGamma = 5.0f;

int3 DDGIPositiveMod(int3 value, int3 count)
{
    return ((value % count) + count) % count;
}

/// <summary>ワールドのセルから格納先 (0〜count-1)。格子が動いても同じセルは同じ場所</summary>
int3 DDGICellToSlot(DDGIVolume volume, int3 cell)
{
    return DDGIPositiveMod(cell, volume.probeCounts);
}

/// <summary>格納先から、今そこにいるプローブのワールドのセル</summary>
int3 DDGISlotToCell(int3 baseCell, int3 counts, int3 slot)
{
    return baseCell + DDGIPositiveMod(slot - baseCell, counts);
}

/// <summary>格納先の通し番号。レイの結果の行と、アトラスでの位置に使う</summary>
uint DDGISlotToIndex(DDGIVolume volume, int3 slot)
{
    return uint(slot.x + volume.probeCounts.x * (slot.y + volume.probeCounts.y * slot.z));
}

int3 DDGIIndexToSlot(DDGIVolume volume, uint index)
{
    int x = int(index) % volume.probeCounts.x;
    int y = (int(index) / volume.probeCounts.x) % volume.probeCounts.y;
    int z = int(index) / (volume.probeCounts.x * volume.probeCounts.y);
    return int3(x, y, z);
}

/// <summary>アトラスでのプローブのタイルの位置。横に x と y、縦に z を並べる</summary>
uint2 DDGISlotToTile(DDGIVolume volume, int3 slot)
{
    return uint2(slot.x + volume.probeCounts.x * slot.y, slot.z + volume.firstTileRow);
}

/// <summary>放射照度のアトラスでのタイルの位置。焼いた GI は段を分けて縦へ積む</summary>
uint2 DDGIIrradianceTile(DDGIVolume volume, int3 slot)
{
    return DDGISlotToTile(volume, slot) + uint2(0, volume.irradianceRowOffset);
}

/// <summary>レイの結果の行。格子ごとに縦へ積んである</summary>
uint DDGIRayRow(DDGIVolume volume, uint probeIndex)
{
    return probeIndex + volume.firstProbe;
}

/// <summary>セルの中心がプローブの位置 (半セルずらす)</summary>
float3 DDGICellToPosition(DDGIVolume volume, int3 cell)
{
    return (float3(cell) + 0.5f) * volume.spacing;
}

float2 DDGISignNotZero(float2 v)
{
    return float2(v.x >= 0.0f ? 1.0f : -1.0f, v.y >= 0.0f ? 1.0f : -1.0f);
}

/// <summary>向きを八面体の平面([-1,1]^2)へ</summary>
float2 DDGIOctEncode(float3 direction)
{
    float3 n = direction / (abs(direction.x) + abs(direction.y) + abs(direction.z));
    float2 uv = n.xy;
    if (n.z < 0.0f)
    {
        uv = (1.0f - abs(n.yx)) * DDGISignNotZero(n.xy);
    }
    return uv;
}

float3 DDGIOctDecode(float2 uv)
{
    float3 n = float3(uv, 1.0f - abs(uv.x) - abs(uv.y));
    if (n.z < 0.0f)
    {
        n.xy = (1.0f - abs(n.yx)) * DDGISignNotZero(n.xy);
    }
    return normalize(n);
}

/// <summary>球面上に n 本を均等に並べたときの i 番目 (球面フィボナッチ)</summary>
float3 DDGISphericalFibonacci(float i, float n)
{
    const float goldenRatio = 1.61803398875f;
    float phi = 6.28318530718f * frac(i * (goldenRatio - 1.0f));
    float cosTheta = 1.0f - (2.0f * i + 1.0f) / n;
    float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

/// <summary>放射照度に混ぜる i 番目のレイの向き (毎フレーム回す)。結果の列は kDDGIFixedRays + i</summary>
float3 DDGIRayDirection(DDGIVolume volume, uint rayIndex)
{
    float3 direction = DDGISphericalFibonacci(float(rayIndex), float(volume.raysPerProbe));
    return float3(dot(volume.rayRotation[0].xyz, direction),
                  dot(volume.rayRotation[1].xyz, direction),
                  dot(volume.rayRotation[2].xyz, direction));
}

/// <summary>向きを回さない i 番目のレイの向き。結果の列は i</summary>
float3 DDGIFixedRayDirection(uint rayIndex)
{
    return DDGISphericalFibonacci(float(rayIndex), float(kDDGIFixedRays));
}

/// <summary>このフレームに受け持つセルが変わったプローブか</summary>
bool DDGIIsProbeReset(DDGIVolume volume, int3 slot)
{
    int3 cell = DDGISlotToCell(volume.baseCell, volume.probeCounts, slot);
    int3 previousCell = DDGISlotToCell(volume.previousBaseCell, volume.probeCounts, slot);
    return volume.resetAll != 0 || any(cell != previousCell);
}

/// <summary>アトラスのUV。外周1texelの縁の内側を引く</summary>
float2 DDGIAtlasUv(uint2 tile, float3 direction, uint interiorTexels, float2 atlasSize)
{
    float2 oct = DDGIOctEncode(direction) * 0.5f + 0.5f;
    float2 pixel = float2(tile) * float(interiorTexels + 2) + 1.0f + oct * float(interiorTexels);
    return pixel / atlasSize;
}

#endif // DDGI_COMMON_HLSLI
