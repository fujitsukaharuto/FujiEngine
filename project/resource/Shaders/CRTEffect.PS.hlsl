struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

cbuffer Constants : register(b0)
{
    float iTime; // 時間 (アプリケーションから渡す)
    float2 iResolution; // 解像度 (アプリケーションから渡す)
}

Texture2D iChannel0 : register(t0); // テクスチャ
SamplerState samplerState : register(s0); // サンプラーステート

float3 scanline(float2 coord, float3 screen)
{
    screen.rgb -= sin(coord.y + (iTime * 29.0)) * 0.02;
    return screen;
}

float2 crt(float2 coord, float bend)
{
    // 中心座標系に変換
    coord = (coord - 0.5) * 2.0;

    // 極座標に変換
    float tht = atan2(coord.y, coord.x);
    float r = length(coord);

    // 中心を歪めない非線形変形
    r /= (1.0 - 0.1 * r * r);

    // 歪みを反映した新しい座標
    coord.x = r * cos(tht);
    coord.y = r * sin(tht);

    // 0.0 - 1.0 の空間に戻す
    coord = (coord / 2.0) + 0.5;

    return coord;
}

float3 sampleSplit(Texture2D tex, float2 coord)
{
    float3 frag;
    frag.r = tex.Sample(samplerState, float2(coord.x - 0.0035 * sin(iTime), coord.y)).r;
    frag.g = tex.Sample(samplerState, float2(coord.x, coord.y)).g;
    frag.b = tex.Sample(samplerState, float2(coord.x + 0.0035 * sin(iTime), coord.y)).b;
    return frag;
}

float4 main(PSInput input) : SV_Target
{
    float2 crtCoords = crt(input.texcoord, 4.0);

    // テクスチャ範囲外をチェック
    if (crtCoords.x < 0.0 || crtCoords.x > 1.0 || crtCoords.y < 0.0 || crtCoords.y > 1.0)
        return float4(0.0, 0.0, 0.0, 1.0);

    // 色チャネルを分割
    float3 color = sampleSplit(iChannel0, crtCoords);

    // スキャンライン効果
    float2 screenSpace = crtCoords * iResolution.xy;
    color = scanline(screenSpace, color);

    float2 centeredUV = input.texcoord - 0.5;
    float vignette = 1.0 - dot(centeredUV, centeredUV) * 2.2;
    vignette = saturate(pow(vignette, 1.5)); // 減光の強さ調整
    color *= vignette;
    
    return float4(color, 1.0);
}