cbuffer Constants : register(b0)
{
    float iTime;
    float2 iResolution;
}

Texture2D iChannel0 : register(t0);
SamplerState samplerState : register(s0);
RWTexture2D<float4> outputTex : register(u0); // 出力先


float3 LinearToSRGB(float3 linearColor)
{
    float3 srgbColor = linearColor;
    // R成分
    if (linearColor.x <= 0.0031308f)
        srgbColor.x = 12.92f * linearColor.x;
    else
        srgbColor.x = 1.055f * pow(linearColor.x, 1.0f / 2.4f) - 0.055f;

    // G成分
    if (linearColor.y <= 0.0031308f)
        srgbColor.y = 12.92f * linearColor.y;
    else
        srgbColor.y = 1.055f * pow(linearColor.y, 1.0f / 2.4f) - 0.055f;

    // B成分
    if (linearColor.z <= 0.0031308f)
        srgbColor.z = 12.92f * linearColor.z;
    else
        srgbColor.z = 1.055f * pow(linearColor.z, 1.0f / 2.4f) - 0.055f;
    return saturate(srgbColor);
}

float3 scanline(float2 coord, float3 screen)
{
    screen.rgb -= sin(coord.y + (iTime * 29.0)) * 0.02;
    return screen;
}

float2 crt(float2 coord, float bend)
{
    //// 中心座標系に変換
    //coord = (coord - 0.5) * 2.0;

    //coord *= 1.1;

    //// 座標の変形
    //coord.x *= 1.0 + pow(abs(coord.y) / bend, 2.0);
    //coord.y *= 1.0 + pow(abs(coord.x) / bend, 2.0);

    //// 0.0 - 1.0 の空間に戻す
    //coord = (coord / 2.0) + 0.5;

    return coord;
}

float3 sampleSplit(Texture2D tex, float2 coord)
{
    float3 frag;
    frag.r = tex.SampleLevel(samplerState, float2(coord.x - 0.005 * sin(iTime), coord.y), 0.0).r;
    frag.g = tex.SampleLevel(samplerState, float2(coord.x, coord.y), 0.0).g;
    frag.b = tex.SampleLevel(samplerState, float2(coord.x + 0.005 * sin(iTime), coord.y), 0.0).b;
    return frag;
}


[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    // 解像度を超えるスレッドは無視
    if (DTid.x >= iResolution.x || DTid.y >= iResolution.y)
        return;

    float2 uv = float2(DTid.xy) / iResolution;

    float2 crtCoords = crt(uv, 3.2);

    // 範囲外チェック
    if (crtCoords.x < 0.0 || crtCoords.x > 1.0 || crtCoords.y < 0.0 || crtCoords.y > 1.0)
        return;

    float3 color = sampleSplit(iChannel0, crtCoords);

    float2 screenSpace = crtCoords * iResolution;
    color = scanline(screenSpace, color);
    color.rgb = LinearToSRGB(color.rgb);
    
    outputTex[DTid.xy] = float4(color, 1.0);
}