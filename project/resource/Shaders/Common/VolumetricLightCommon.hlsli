#ifndef VOLUMETRIC_LIGHT_COMMON_HLSLI
#define VOLUMETRIC_LIGHT_COMMON_HLSLI

// 光の筋の CS 側の共通部分。縦横 1/4 の解像度で求める
// a はカメラからの奥行き。ぼかしと拡大で物の境目をまたがないのに使う

// VolumetricLightPass.h の VolumetricParam と並びが一致していること
cbuffer VolumetricParam : register(b2)
{
    uint2 lowSize;        // 計算する解像度
    uint2 screenSize;     // 画面の解像度
    float density;        // 1m あたりの散乱
    float anisotropy;     // 散乱の向きの偏り。1に近いほど光の進む向きへ強く散る
    float maxDistance;    // カメラからこの距離までを見る
    float intensity;      // 散った光の倍率 (UE の Volumetric Scattering Intensity)
    float2 depthToView;   // 深度 d から奥行きへ: y / (d - x)
    float sunScale;       // 太陽の光の筋の倍率。0 でレイを飛ばさない
    float localLightScale; // 影なしのライトが霧でにじむ倍率
};

/// <summary>深度からビュー空間の奥行き</summary>
float VolumetricViewDepth(float depth)
{
    // 分母は常に負
    return depthToView.y / min(depth - depthToView.x, -1e-6f);
}

#endif // VOLUMETRIC_LIGHT_COMMON_HLSLI
