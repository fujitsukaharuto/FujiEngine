// 光の筋(ゴッドレイ)。太陽のまわりの空を光源とみなし、各画素から太陽へ向かって画面上を辿って、
// 物に遮られずに届く光を足す。空の絵には依らないので、スカイボックスが無いシーンでも出る

Texture2D InputTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

// OffscreenManager.h の GodRayParams と並びが一致していること
cbuffer GodRayParams : register(b0)
{
    float2 sunUV; // 太陽の画面上の位置。画面の外のこともある
    float intensity; // 0以下なら何もしない(太陽が背後にあるときもここが0で来る)
    float rayLength; // 太陽までの道のりのうち、どこまで辿るか。1で太陽まで
    float3 color;
    float sunSize; // 光源とみなす範囲の半径。画面の高さが1
};

static const int kStepCount = 32;

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
    if (intensity <= 0.0f)
    {
        outputTexture[coord] = float4(scene, 1.0f);
        return;
    }

    float2 size = float2(width, height);
    float2 uv = (coord + 0.5f) / size;
    float2 aspect = float2(size.x / size.y, 1.0f);
    float2 stepUV = (sunUV - uv) * (rayLength / kStepCount);

    // 辿り始めを画素ごとにずらす。揃えたままだと刻みの縞が同心円に出る
    float noise = frac(52.9829189f * frac(dot(float2(coord), float2(0.06711056f, 0.00583715f))));

    float light = 0.0f;
    for (int i = 0; i < kStepCount; ++i)
    {
        float t = i + noise;
        float2 p = uv + stepUV * t;

        // 画面の外は分からないので、遮る物は無いとみなす(太陽が少し外へ出ても筋が急に消えない)
        float visible = 1.0f;
        if (all(p >= 0.0f) && all(p < 1.0f))
        {
            // 何も描かれていない画素(深度がクリア値のまま)が空
            visible = gDepthTexture[uint2(p * size)] >= 1.0f ? 1.0f : 0.0f;
        }

        float glow = saturate(1.0f - length((p - sunUV) * aspect) / sunSize);
        // 遠くで拾った光ほど弱く届く
        float weight = 1.0f - t / kStepCount;
        light += visible * glow * glow * weight;
    }
    // 重みの合計はおよそ kStepCount / 2
    light *= 2.0f / kStepCount;

    outputTexture[coord] = float4(scene + color * (intensity * light), 1.0f);
}
