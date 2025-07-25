cbuffer Constants : register(b0)
{
    float iTime;
    float2 iResolution;
}

Texture2D<float4> iChannel0 : register(t0);
SamplerState samplerState : register(s0);
RWTexture2D<float4> outputTexture : register(u0);

float3 scanline(float2 coord, float3 screen)
{
    screen.rgb -= sin(coord.y + (iTime * 29.0)) * 0.02;
    return screen;
}

float2 crt(float2 coord, float bend)
{
    coord = (coord - 0.5) * 2.0;

    float tht = atan2(coord.y, coord.x);
    float r = length(coord);

    r /= (1.0 - 0.1 * r * r);

    coord.x = r * cos(tht);
    coord.y = r * sin(tht);

    coord = (coord / 2.0) + 0.5;
    return coord;
}

float3 sampleSplit(Texture2D tex, float2 coord)
{
    float3 frag;
    frag.r = tex.SampleLevel(samplerState, float2(coord.x - 0.0035 * sin(iTime), coord.y), 0).r;
    frag.g = tex.SampleLevel(samplerState, float2(coord.x, coord.y), 0).g;
    frag.b = tex.SampleLevel(samplerState, float2(coord.x + 0.0035 * sin(iTime), coord.y), 0).b;
    return frag;
}

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

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= (uint) iResolution.x || DTid.y >= (uint) iResolution.y)
        return;

    float2 uv = (float2(DTid.xy) + 0.5) / iResolution.xy;

    float2 crtCoords = crt(uv, 4.0);
    if (crtCoords.x < 0.0 || crtCoords.x > 1.0 || crtCoords.y < 0.0 || crtCoords.y > 1.0)
    {
        outputTexture[DTid.xy] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float3 color = sampleSplit(iChannel0, crtCoords);
    float2 screenSpace = crtCoords * iResolution.xy;
    color = scanline(screenSpace, color);

    float2 centeredUV = uv - 0.5;
    float vignette = 1.0 - dot(centeredUV, centeredUV) * 2.2;
    vignette = saturate(pow(vignette, 1.5));
    color *= vignette;
    color.rgb = LinearToSRGB(color.rgb);
    
    outputTexture[DTid.xy] = float4(color, 1.0);
}