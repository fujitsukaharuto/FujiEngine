struct VS_INPUT
{
    float4 pos : POSITION; // 頂点位置
    float2 uv : TEXCOORD; // UV座標
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION; // スクリーン空間座標
    float2 uv : TEXCOORD0; // UV座標
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

    coord *= 1.1;

    // 座標の変形
    coord.x *= 1.0 + pow(abs(coord.y) / bend, 2.0);
    coord.y *= 1.0 + pow(abs(coord.x) / bend, 2.0);

    // 0.0 - 1.0 の空間に戻す
    coord = (coord / 2.0) + 0.5;

    return coord;
}

float3 sampleSplit(Texture2D tex, float2 coord)
{
    float3 frag;
    frag.r = tex.Sample(samplerState, float2(coord.x - 0.01 * sin(iTime), coord.y)).r;
    frag.g = tex.Sample(samplerState, float2(coord.x, coord.y)).g;
    frag.b = tex.Sample(samplerState, float2(coord.x + 0.01 * sin(iTime), coord.y)).b;
    return frag;
}

float4 main(VS_OUTPUT input) : SV_Target
{
    float2 crtCoords = crt(input.uv, 3.2);

    // テクスチャ範囲外をチェック
    if (crtCoords.x < 0.0 || crtCoords.x > 1.0 || crtCoords.y < 0.0 || crtCoords.y > 1.0)
        discard;

    // 色チャネルを分割
    float3 color = sampleSplit(iChannel0, crtCoords);

    // スキャンライン効果
    float2 screenSpace = crtCoords * iResolution.xy;
    color = scanline(screenSpace, color);

    return float4(color, 1.0);
}