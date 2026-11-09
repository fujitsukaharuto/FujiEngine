// 局所トーンマップの地図を作る (1/2)。16x16 画素の区画ごとに log2 の明るさの平均を取る

Texture2D<float4> InputTexture : register(t0);
RWTexture2D<float> outputTexture : register(u0);

static const float3 kLumaWeight = float3(0.2126f, 0.7152f, 0.0722f);
static const uint kBlockSize = 16;
// 扱う範囲を自動露出と同じにする
static const float kMinLog = -12.0f;
static const float kMaxLog = 4.0f;

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

    uint width;
    uint height;
    InputTexture.GetDimensions(width, height);

    // 2画素おきに 8x8 点を見る
    float sum = 0.0f;
    float count = 0.0f;
    uint2 origin = dispatchID.xy * kBlockSize;
    for (uint y = 0; y < kBlockSize; y += 2)
    {
        for (uint x = 0; x < kBlockSize; x += 2)
        {
            uint2 pixel = origin + uint2(x, y);
            if (all(pixel < uint2(width, height)))
            {
                float luminance = dot(InputTexture[pixel].rgb, kLumaWeight);
                sum += clamp(log2(max(luminance, 1e-8f)), kMinLog, kMaxLog);
                count += 1.0f;
            }
        }
    }
    outputTexture[dispatchID.xy] = (count > 0.0f) ? sum / count : 0.0f;
}
