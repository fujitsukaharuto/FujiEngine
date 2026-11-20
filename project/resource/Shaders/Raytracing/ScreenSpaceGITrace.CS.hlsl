#include "../Object/Object3d.hlsli"
#include "../Common/ScreenSpaceGICommon.hlsli"
#include "../Common/AerialPerspective.hlsli"
#include "../Common/VolumetricFog.hlsli"
#include "../Common/RayTracedShadow.hlsli"

// 画素ごとに半球へ数本のレイを深度の上で進め、当たった所の前のフレームの色を集める。乱数は使わない

ConstantBuffer<AllLights> gLights : register(b0);
Texture2D<float4> gSSGISceneColor : register(t2, space13); // 前のフレームの、不透明の物を描き終えた画面
RWTexture2D<float4> gSSGITraceOut : register(u0);           // rgb=照り返し, a=遮られた割合
SamplerState gSampler : register(s0);

// 4x4 の画素で方向を分け合う。ぼかしが1周期ぶんを混ぜる
static const uint kPatternSize = 4;
static const uint kRaysPerPixel = 4;
static const uint kStepsPerRay = 6;
// 面の奥へこれ以上潜った所は当たりにしない。カメラからの距離に対する割合
static const float kThicknessRatio = 0.03f;
static const float kMinThickness = 0.3f;
// 自分の面からこれ以上離れていない所は当たりにしない (m)
static const float kSelfHitDistance = 0.05f;
// 画面の端からこの割合の内側で弱める
static const float kEdgeFade = 0.05f;
// これより霧が濃い所の色は使わない
static const float kMaxFog = 0.9f;

float LinearDepth(float depth)
{
    return depthToViewZ.x / (depth - depthToViewZ.y);
}

float2 ClipToUV(float4 clip)
{
    float2 ndc = clip.xy / clip.w;
    return float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
}

float EdgeWeight(float2 uv)
{
    float edge = min(min(uv.x, 1.0f - uv.x), min(uv.y, 1.0f - uv.y));
    return saturate(edge / kEdgeFade);
}

/// <summary>当たった所の、前のフレームの画面の色。霧と空気遠近は乗ったまま</summary>
/// <returns>色が取れたか</returns>
bool LoadHitColor(float3 hitPosition, out float3 color)
{
    color = float3(0.0f, 0.0f, 0.0f);
    float4 clip = mul(float4(hitPosition, 1.0f), sceneColorViewProj);
    if (clip.w <= 1e-4f)
    {
        return false;
    }
    float2 uv = ClipToUV(clip);
    if (any(uv < 0.0f) || any(uv >= 1.0f))
    {
        return false;
    }
    int2 pixel = min(int2(uv * float2(screenSize)), int2(screenSize) - 1);
    color = gSSGISceneColor.Load(int3(pixel, 0)).rgb;
    return true;
}

// 歩き始めのずらし (4x4 ベイヤー)。ぼかしで混ざると歩幅の段が消える
static const float kStepOffsets[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };

/// <summary>1本のレイを深度の上で進める</summary>
/// <param name="clipStart">レイの出発点のクリップ座標</param>
/// <param name="stepOffset">歩く位置のずらし (0〜1)</param>
/// <param name="weight">当たった所の重み。遮った割合と照り返しの両方に掛かる</param>
/// <param name="isLit">color を取れたか</param>
/// <returns>当たったか</returns>
bool MarchScreenRay(uint2 pixel, float3 position, float3 normal, float4 clipStart, float3 direction, float stepOffset,
                    out float3 color, out float weight, out bool isLit)
{
    color = float3(0.0f, 0.0f, 0.0f);
    weight = 0.0f;
    isLit = false;
    // クリップ座標はレイの上で線形に変わる
    float4 clipStep = mul(float4(direction, 0.0f), viewProj);

    // 深度は全部の歩のぶんを先に読む。1歩ごとに読み終わりを待たない
    int2 taps[kStepsPerRay];
    float rayZ[kStepsPerRay];
    float depths[kStepsPerRay];
    float2 uvs[kStepsPerRay];
    // 画面の外へ出た最初の歩。そこから先は見ない
    uint stepCount = kStepsPerRay;
    [unroll]
    for (uint i = 0; i < kStepsPerRay; i++)
    {
        // 手前ほど細かく進む
        float s = (float(i) + stepOffset) / float(kStepsPerRay);
        float4 clip = clipStart + clipStep * (maxDistance * s * s);
        float2 uv = ClipToUV(clip);
        bool isInside = clip.w > 1e-4f && all(uv >= 0.0f) && all(uv < 1.0f);
        stepCount = min(stepCount, isInside ? kStepsPerRay : i);
        taps[i] = clamp(int2(uv * float2(screenSize)), int2(0, 0), int2(screenSize) - 1);
        rayZ[i] = clip.w;
        uvs[i] = uv;
        depths[i] = gGBufferDepth.Load(int3(taps[i], 0));
    }

    [unroll]
    for (uint j = 0; j < kStepsPerRay; j++)
    {
        if (j >= stepCount)
        {
            return false;
        }
        if (all(taps[j] == int2(pixel)) || depths[j] >= 1.0f)
        {
            continue;
        }
        float sceneZ = LinearDepth(depths[j]);
        float behind = rayZ[j] - sceneZ;
        if (behind <= 0.0f || behind > max(kMinThickness, sceneZ * kThicknessRatio))
        {
            continue;
        }
        float3 hitPosition = ReconstructWorldPos(uint2(taps[j]), screenSize, depths[j], invViewProj);
        if (dot(normal, hitPosition - position) < kSelfHitDistance)
        {
            continue;
        }

        // 遠くで当たったものほど弱める
        float s = (float(j) + stepOffset) / float(kStepsPerRay);
        float falloff = 1.0f - s * s;
        weight = EdgeWeight(uvs[j]) * falloff * falloff;
        // 裏から当たった面は遮るだけ
        float3 hitNormal = gGBufferNormal.Load(int3(taps[j], 0)).xyz;
        if (dot(hitNormal, direction) < 0.0f)
        {
            isLit = LoadHitColor(hitPosition, color);
        }
        return true;
    }
    return false;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (any(pixel >= screenSize))
    {
        return;
    }

    float3 normal;
    float depth;
    if (useSceneColor == 0 || !LoadSurface(gGBufferNormal, gGBufferDepth, int2(pixel), normal, depth))
    {
        gSSGITraceOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float3 position = ReconstructWorldPos(pixel, screenSize, depth, invViewProj);
    float3 origin = position + normal * ReconstructedRayBias(distance(position, cameraPos));
    float4 clipStart = mul(float4(origin, 1.0f), viewProj);

    // 向きは法線から決まる接空間で固定。カメラが動いても変わらない
    float3 tangent;
    float3 bitangent;
    BuildOrthonormalBasis(normal, tangent, bitangent);
    uint patternIndex = (pixel.x % kPatternSize) + (pixel.y % kPatternSize) * kPatternSize;
    const uint patternCount = kPatternSize * kPatternSize;
    float stepOffset = (kStepOffsets[patternIndex] + 0.5f) / float(patternCount);

    // 画面の色に乗った霧と空気遠近は、当たった所でなくこの画素の値で外す
    float3 toPixel = position - cameraPos;
    float2 pixelUV = (float2(pixel) + 0.5f) / float2(screenSize);
    float pixelExclude = VolumetricFogExcludeDistance(toPixel);
    float pixelDistance = length(toPixel);
    float3 pixelDirection = toPixel / max(pixelDistance, 1e-6f);
    float fogAmount = FogAmountFrom(gLights.fog, cameraPos, position, pixelExclude);
    float volumetricTransmittance;
    float3 volumetricScatter;
    VolumetricFogTermsAt(gSampler, pixelUV, VolumetricFogViewDepth(toPixel), volumetricTransmittance, volumetricScatter);
    // 2つの霧を合わせて、元の色がどれだけ残っているか
    float localFogTransmittance = LocalFogVolumesTransmittance(cameraPos, pixelDirection, pixelDistance, pixelExclude);
    if (1.0f - volumetricTransmittance * localFogTransmittance * (1.0f - fogAmount) > kMaxFog)
    {
        gSSGITraceOut[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float3 fogColor = FogColor(gLights.fog, toPixel);
    float3 aerialTransmittance;
    float3 aerialScatter;
    if (!AerialPerspectiveTermsAt(gLights.fog, gSampler, toPixel, pixelUV,
                                  aerialTransmittance, aerialScatter))
    {
        aerialTransmittance = float3(1.0f, 1.0f, 1.0f);
        aerialScatter = float3(0.0f, 0.0f, 0.0f);
    }

    float3 colorSum = float3(0.0f, 0.0f, 0.0f);
    float litWeight = 0.0f;
    float occlusion = 0.0f;
    for (uint r = 0; r < kRaysPerPixel; r++)
    {
        // 1画素の中で仰角が散らばるよう、画素の番号を下の桁に置く
        float2 xi = Hammersley(r * patternCount + patternIndex, kRaysPerPixel * patternCount);
        // cos で重みづけした半球
        float sinTheta = sqrt(xi.x);
        float cosTheta = sqrt(1.0f - xi.x);
        float phi = kSamplingTwoPI * xi.y;
        float3 direction = normalize(tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + normal * cosTheta);

        float3 color;
        float weight;
        bool isLit;
        if (MarchScreenRay(pixel, position, normal, clipStart, direction, stepOffset, color, weight, isLit))
        {
            occlusion += weight;
            if (isLit)
            {
                colorSum += color * weight;
                litWeight += weight;
            }
        }
    }

    // 前方描画の 空気遠近 → 式の霧 → 表の霧 と逆の順に外す
    float3 radianceSum = (colorSum - volumetricScatter * litWeight) / max(volumetricTransmittance, 1e-3f);
    for (int v = int(gVolumetricFog.localFogVolumeCount) - 1; v >= 0; v--)
    {
        float localTransmittance;
        float3 localScatter;
        if (LocalFogVolumeTerms(gLocalFogVolumes[v], cameraPos, pixelDirection, pixelExclude, pixelDistance, localTransmittance, localScatter))
        {
            radianceSum = (radianceSum - localScatter * litWeight) / max(localTransmittance, 1e-3f);
        }
    }
    radianceSum = (radianceSum - fogColor * (fogAmount * litWeight)) / (1.0f - fogAmount);
    radianceSum = max((radianceSum - aerialScatter * litWeight) / max(aerialTransmittance, 1e-3f), 0.0f);

    float inverseCount = 1.0f / float(kRaysPerPixel);
    gSSGITraceOut[pixel] = float4(radianceSum * (inverseCount * intensity), occlusion * inverseCount);
}
