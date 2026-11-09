#ifndef RAIN_HLSLI
#define RAIN_HLSLI

// 雨 (RainRenderer)。雨粒の番号からカメラのまわりの箱の中の位置を決める。位置は世界に固定

// RainRenderer.h の RainParams と並びが一致していること
cbuffer RainParams : register(b0)
{
    float4x4 rainViewProjection;
    float3 rainCameraPosition;
    float rainTime;          // 秒
    float3 rainAmbient;      // 空からの明るさ
    float rainBoxSize;       // カメラのまわりの箱の1辺 (m)
    float2 rainWind;         // 横に流れる速さ (m/s、xz)
    float rainFallSpeed;     // 落ちる速さ (m/s)
    float rainStreakLength;  // 雨粒の筋の長さ (m)
    float rainWidth;         // 筋の太さ (m)
    float rainBrightness;    // 照らされた雨粒の明るさの倍率
    float2 rainScreenSize;   // 描画の解像度
};

struct RainVertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR0;   // 雨粒が受けている光 × 薄さ
    float across : TEXCOORD0; // 筋の太さ方向の位置 (-1〜1)
    float along : TEXCOORD1;  // 筋の長さ方向の位置 (0=先頭, 1=後ろ)
};

#endif // RAIN_HLSLI
