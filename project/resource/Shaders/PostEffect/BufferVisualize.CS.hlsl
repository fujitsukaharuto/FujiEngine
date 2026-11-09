// 描画の途中のバッファを見える色に直して写す (BufferVisualizer)。出力はリニア

Texture2D<float4> InputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

// BufferVisualizer.h の BufferVisualizeParams と並びが一致していること
cbuffer BufferVisualizeParams : register(b0)
{
    uint mode; // BufferVisualizer::Mode
    float exposure; // 表示に掛ける露光
    float2 depthToView; // 深度 d から距離へ: y / (d - x)
};

static const uint kModeColor = 0; // HDR の色。露光を掛けて Reinhard で丸める
static const uint kModeLuminance = 1; // 明るさを段 (EV) ごとの色分けで
static const uint kModeDepth = 2; // カメラからの距離。手前ほど白い
static const uint kModeNormal = 3; // ワールドの法線を 0〜1 へ
static const uint kModeScalar = 4; // 1チャンネルの 0〜1 (AO・影)
static const uint kModeLogLuminance = 5; // log2 の明るさを段ごとの色分けで

static const float3 kLumaWeight = float3(0.2126f, 0.7152f, 0.0722f);
// 色分けの範囲。中間の灰色 (0.18) が真ん中の緑
static const float kHeatMinEV = -6.0f;
static const float kHeatMaxEV = 6.0f;
// 距離の表示の奥の端
static const float kMaxDistance = 200.0f;

/// <summary>0〜1 を 青→水色→緑→黄→赤 へ (Turbo の近似)</summary>
float3 HeatColor(float t)
{
    t = saturate(t);
    float3 color = float3(
        saturate(1.5f - abs(4.0f * t - 3.0f)),
        saturate(1.5f - abs(4.0f * t - 2.0f)),
        saturate(1.5f - abs(4.0f * t - 1.0f)));
    // sRGB の色なのでリニアへ戻す
    return pow(color, 2.2f);
}

float3 HeatFromLog(float logLuminance)
{
    float ev = logLuminance + log2(max(exposure, 1e-6f)) - log2(0.18f);
    return HeatColor((ev - kHeatMinEV) / (kHeatMaxEV - kHeatMinEV));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint outWidth;
    uint outHeight;
    outputTexture.GetDimensions(outWidth, outHeight);
    if (any(dispatchID.xy >= uint2(outWidth, outHeight)))
    {
        return;
    }

    // 大きさの違う入力も同じ位置の画素を拾う
    uint width;
    uint height;
    InputTexture.GetDimensions(width, height);
    float2 uv = (float2(dispatchID.xy) + 0.5f) / float2(outWidth, outHeight);
    uint2 pixel = min(uint2(uv * float2(width, height)), uint2(width, height) - 1);
    float4 value = InputTexture[pixel];

    float3 color;
    switch (mode)
    {
        case kModeLuminance:
            color = HeatFromLog(log2(max(dot(value.rgb, kLumaWeight), 1e-8f)));
            break;
        case kModeDepth:
        {
            // 何も描かれていない所 (深度がクリア値) は黒
            if (value.r >= 1.0f)
            {
                color = 0.0f;
                break;
            }
            float distance = depthToView.y / (value.r - depthToView.x);
            float t = saturate(log2(1.0f + max(distance, 0.0f)) / log2(1.0f + kMaxDistance));
            color = pow(1.0f - t, 2.2f);
            break;
        }
        case kModeNormal:
            color = (dot(value.xyz, value.xyz) > 1e-6f) ? pow(saturate(value.xyz * 0.5f + 0.5f), 2.2f) : 0.0f;
            break;
        case kModeScalar:
            // sRGB の尺度で並べる
            color = pow(saturate(value.r), 2.2f);
            break;
        case kModeLogLuminance:
            color = HeatFromLog(value.r);
            break;
        default:
        {
            float3 hdr = max(value.rgb, 0.0f) * exposure;
            color = hdr / (1.0f + hdr);
            break;
        }
    }
    outputTexture[dispatchID.xy] = float4(color, 1.0f);
}
