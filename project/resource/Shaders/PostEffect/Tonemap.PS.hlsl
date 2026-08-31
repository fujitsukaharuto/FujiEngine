// 画面へ出す最終パス(Pipe::None 専用)。リニアHDRを露光・トーンマップして表示レンジへ落とす。
// 出力先のRTVが _SRGB なのでガンマは自前で掛けないこと

Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer TonemapParams : register(b0)
{
    float exposure; // トーンマップ前に掛ける露光倍率
    uint tonemapMode; // 0=なし / 1=Reinhard / 2=ACES
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// Reinhard。最も単純だがハイライトが平坦になりやすい
float3 Reinhard(float3 x)
{
    return x / (1.0f + x);
}

// ACES のフィルミックカーブ近似 (Krzysztof Narkowicz)。1.0 -> 0.80 / 2.0 -> 0.93
float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PSInput input) : SV_TARGET
{
    // サンプラがWRAPなので、縮小表示のときに端の半テクセルが反対側を拾わないよう内側へ寄せる
    float2 size;
    g_InputTexture.GetDimensions(size.x, size.y);
    float2 uv = clamp(input.texcoord, 0.5f / size, 1.0f - 0.5f / size);

    float4 color = g_InputTexture.Sample(g_Sampler, uv);

    float3 hdr = color.rgb * exposure;

    float3 mapped;
    switch (tonemapMode)
    {
        case 1:
            mapped = Reinhard(hdr);
            break;
        case 2:
            mapped = ACESFilm(hdr);
            break;
        default:
            mapped = hdr; // 素通し。HDR化の切り分け用
            break;
    }

    return float4(saturate(mapped), 1.0f);
}
