// 光の筋を画面へ足す。1/4 の解像度の結果を奥行きの近い点だけで引き伸ばす

Texture2D InputTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
Texture2D<float4> gVolumetricLight : register(t2); // rgb=散った光, a=奥行き
RWTexture2D<float4> outputTexture : register(u0);

// OffscreenManager.h の VolumetricCompositeParams と並びが一致していること
cbuffer VolumetricCompositeParams : register(b0)
{
    float2 depthToView; // 深度 d から奥行きへ: y / (d - x)
    uint2 lowSize;      // 光の筋の解像度
    uint enabled;       // 0 なら素通し
    float3 pad;
};

static const float kDepthTolerance = 0.1f;

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
    if (enabled == 0)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }

    float viewDepth = depthToView.y / min(gDepthTexture[coord] - depthToView.x, -1e-6f);
    float2 lowPosition = (float2(coord) + 0.5f) / float2(width, height) * float2(lowSize) - 0.5f;
    int2 base = int2(floor(lowPosition));
    float2 f = lowPosition - float2(base);

    float3 sum = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    // 4点とも離れていたら奥行きのいちばん近い点
    float3 nearest = float3(0.0f, 0.0f, 0.0f);
    float nearestDifference = 1e30f;
    for (int i = 0; i < 4; ++i)
    {
        int2 offset = int2(i & 1, i >> 1);
        int2 tap = clamp(base + offset, int2(0, 0), int2(lowSize) - 1);
        float4 value = gVolumetricLight.Load(int3(tap, 0));
        float2 bilinear = lerp(1.0f - f, f, float2(offset));
        float difference = abs(value.a - viewDepth);
        float weight = bilinear.x * bilinear.y * exp(-difference / (kDepthTolerance * viewDepth + 0.1f));
        sum += value.rgb * weight;
        weightSum += weight;
        if (difference < nearestDifference)
        {
            nearestDifference = difference;
            nearest = value.rgb;
        }
    }
    float3 light = weightSum > 1e-3f ? sum / weightSum : nearest;
    outputTexture[coord] = float4(scene + light, 1.0f);
}
