// 局所トーンマップの地図を作る (2/2)。明るさの近い区画どうしだけでならす

Texture2D<float> InputTexture : register(t0);
RWTexture2D<float> outputTexture : register(u0);

// 9x9 区画 (画面の約 144 画素四方) を見る
static const int kRadius = 4;
static const float kSpatialSigma = 2.5f;
// 1段 (1EV) 離れた区画は重みが約6割
static const float kRangeSigma = 1.0f;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    outputTexture.GetDimensions(width, height);
    if (any(dispatchID.xy >= uint2(width, height)))
    {
        return;
    }

    float center = InputTexture[dispatchID.xy];
    float sum = 0.0f;
    float weightSum = 0.0f;
    for (int y = -kRadius; y <= kRadius; ++y)
    {
        for (int x = -kRadius; x <= kRadius; ++x)
        {
            int2 texel = clamp(int2(dispatchID.xy) + int2(x, y), int2(0, 0), int2(width, height) - 1);
            float value = InputTexture[texel];
            float difference = value - center;
            float weight = exp(-float(x * x + y * y) / (2.0f * kSpatialSigma * kSpatialSigma)) *
                exp(-difference * difference / (2.0f * kRangeSigma * kRangeSigma));
            sum += value * weight;
            weightSum += weight;
        }
    }
    outputTexture[dispatchID.xy] = sum / weightSum;
}
