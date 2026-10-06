// 被写界深度。ピントの合う距離から離れた物ほどぼかす。
// 深度を書かない物(加算の粒子など)は、その奥にある物の距離でぼける

Texture2D InputTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

// OffscreenManager.h の DepthOfFieldParams と並びが一致していること
cbuffer DepthOfFieldParams : register(b0)
{
    float focusDistance; // ピントの合う距離
    float focusRange; // ピントの前後、この幅まではぼかさない
    float falloff; // そこからこの距離をかけて最大のぼけに達する
    float maxBlur; // ぼけの最大半径(画素)。0以下なら何もしない
    float2 depthToView; // 深度 d からカメラ空間の距離へ: y / (d - x)
    float2 pad;
};

static const int kSampleCount = 32;
static const float kGoldenAngle = 2.39996323f;

// その深度の物のぼけの半径(画素)
float BlurRadius(float depth)
{
    // x は 1 よりわずかに大きく y は負なので、分母は常に負
    float viewZ = depthToView.y / min(depth - depthToView.x, -1e-6f);
    return maxBlur * saturate((abs(viewZ - focusDistance) - focusRange) / max(falloff, 1e-4f));
}

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

    float3 color = InputTexture[coord].rgb;
    if (maxBlur <= 0.0f)
    {
        outputTexture[coord] = float4(color, 1.0f);
        return;
    }

    float centerDepth = gDepthTexture[coord];
    float centerRadius = BlurRadius(centerDepth);

    // 円の向きを画素ごとに回す。揃えたままだと点の並びが模様に見える
    float noise = frac(52.9829189f * frac(dot(float2(coord), float2(0.06711056f, 0.00583715f))));
    int2 maxCoord = int2(width, height) - 1;

    float total = 1.0f;
    for (int i = 0; i < kSampleCount; ++i)
    {
        // 円の中に均等に散らす
        float radius = maxBlur * sqrt((i + 0.5f) / kSampleCount);
        float angle = (i + noise) * kGoldenAngle;
        int2 p = clamp(int2(coord) + int2(round(float2(cos(angle), sin(angle)) * radius)), int2(0, 0), maxCoord);

        float sampleDepth = gDepthTexture[p];
        float sampleRadius = BlurRadius(sampleDepth);
        // 奥の物のぼけは、手前のくっきりした物の上までは広がらない
        if (sampleDepth > centerDepth)
        {
            sampleRadius = min(sampleRadius, centerRadius * 2.0f);
        }

        // その物のぼけがここまで届いていれば色を拾う。届いていなければ今までの平均で埋める
        float reach = smoothstep(radius - 0.5f, radius + 0.5f, sampleRadius);
        color += lerp(color / total, InputTexture[p].rgb, reach);
        total += 1.0f;
    }

    outputTexture[coord] = float4(color / total, 1.0f);
}
