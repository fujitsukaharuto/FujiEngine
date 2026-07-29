
// サンプラー
SamplerState InputSampler : register(s0);

// 出力テクスチャ (UAV)
RWTexture2D<float4> outputTexture : register(u0);

cbuffer Constants : register(b0)
{
    float iTime;
    float2 iResolution;
}

float rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

[numthreads(8, 8, 1)] // スレッドグループのサイズ (X, Y, Z)
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // 現在のピクセルの座標
    int2 coords = dispatchThreadID.xy;
    float randColor = rand2dTo1d(coords * iTime);
    float3 outColor = float3(randColor, randColor, randColor);
    
    // 結果を出力テクスチャに書き込む
    outputTexture[coords] = float4(outColor, 1.0);
}
