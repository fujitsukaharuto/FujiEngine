#ifndef RAIN_CURTAIN_HLSLI
#define RAIN_CURTAIN_HLSLI

// 遠くの雨のカーテン。カメラから決まった距離の層に筋を描く

// 層の数。RainRenderer.h の kCurtainLayers_ と同じ値であること
static const uint kRainCurtainLayers = 4;

struct RainCurtainOutput
{
    float4 position : SV_POSITION;
    float2 ndc : TEXCOORD0;
    nointerpolation uint layer : TEXCOORD1;
};

/// <summary>層の距離 (m)。奥ほど間をあける</summary>
float RainCurtainDistance(uint layer)
{
    static const float kScale[kRainCurtainLayers] = { 1.0f, 1.6f, 2.6f, 4.2f };
    return curtainStart * kScale[layer];
}

#endif // RAIN_CURTAIN_HLSLI
