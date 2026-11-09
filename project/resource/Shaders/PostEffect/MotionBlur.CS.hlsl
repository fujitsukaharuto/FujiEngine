// カメラのモーションブラー。前のフレームのカメラで写した位置との差の向きにぼかす
// 動く物それぞれの速さは見ない

Texture2D InputTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

// OffscreenManager.h の MotionBlurParams と並びが一致していること
cbuffer MotionBlurParams : register(b0)
{
    float4x4 currentToPrevious; // 今のクリップ座標から前のフレームのクリップ座標へ
    float strength; // シャッターの開いている割合。0以下なら何もしない
    float maxBlur; // ぼかす幅の上限(画素)
    float2 pad;
};

static const int kSampleCount = 16;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    uint width, height;
    InputTexture.GetDimensions(width, height);
    if (coord.x >= width || coord.y >= height)
    {
        return;
    }

    float3 scene = InputTexture[coord].rgb;
    if (strength <= 0.0f)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }

    float2 size = float2(width, height);
    float2 uv = (coord + 0.5f) / size;
    // 空は遠くの面として扱う
    float depth = gDepthTexture[coord];
    float4 previous = mul(float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth, 1.0f), currentToPrevious);
    if (previous.w <= 1e-5f)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }
    float2 previousUV = float2(previous.x / previous.w * 0.5f + 0.5f, 0.5f - previous.y / previous.w * 0.5f);
    float2 velocity = (uv - previousUV) * size * strength;
    float speed = length(velocity);
    if (speed < 0.5f)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }
    velocity *= min(speed, maxBlur) / speed;

    // 動いた向きの前後へ等間隔に引く
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < kSampleCount; ++i)
    {
        float t = (i + 0.5f) / kSampleCount - 0.5f;
        int2 tap = clamp(int2(float2(coord) + 0.5f + velocity * t), int2(0, 0), int2(width - 1, height - 1));
        sum += InputTexture[tap].rgb;
    }
    outputTexture[coord] = float4(sum / kSampleCount, 1.0f);
}
