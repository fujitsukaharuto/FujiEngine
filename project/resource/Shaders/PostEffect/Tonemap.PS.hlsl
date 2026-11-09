// 画面へ出す最終パス(Pipe::None 専用)。リニアHDRを露光・トーンマップして表示レンジへ落とす。
// 出力先のRTVが _SRGB なのでガンマは自前で掛けないこと

Texture2D g_InputTexture : register(t0);
// 自動露出の結果。[0] が倍率 (PostEffect/AutoExposure*.CS.hlsl)
StructuredBuffer<float> gAutoExposure : register(t1);
// 局所トーンマップのまわりの明るさ (log2、露出前)。16x16 画素ごとに1つ
Texture2D<float> gLocalTonemap : register(t2);
SamplerState g_Sampler : register(s0);

// OffscreenManager.h の TonemapParams と並びが一致していること
cbuffer TonemapParams : register(b0)
{
    float exposure; // トーンマップ前に掛ける露光倍率
    uint tonemapMode; // 0=なし / 1=Reinhard / 2=ACES / 3=AgX
    uint enableFXAA; // 0以外なら輪郭のギザギザをならす
    uint enableDither; // 0以外なら8bitへ落とすときの縞を散らす
    float saturation; // 彩度。1でそのまま、0で白黒
    float contrast; // コントラスト。1でそのまま
    float sharpen; // 輪郭をくっきりさせる強さ。0で何もしない
    float vignette; // 画面の四隅を暗くする強さ。0で何もしない
    float3 shadowTint; // 暗い所に掛ける色。白でそのまま
    uint autoExposure; // 0以外なら自動露出の倍率を掛ける
    float3 highlightTint; // 明るい所に掛ける色。白でそのまま
    float chromaticAberration; // 色収差の強さ。0で何もしない
    float3 whiteBalance; // 色温度の倍率
    float tonemapPad;
    float purkinje; // プルキンエ現象の強さ。0で何もしない
    float purkinjeLuminance; // この明るさ (露出後) より暗い所から寄せる
    float localTonemap; // 局所トーンマップの強さ。0で何もしない
    float localTonemapDetail; // 局所トーンマップで残す細部の濃さ。1でそのまま
    float filmGrain; // フィルムの粒子の強さ。0で何もしない
    float fade; // 褪せたフィルムのようにする強さ。0で何もしない
    float grainSeed; // 粒子の模様をフレームごとに変える種
    float filmPad;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

static const float3 kLumaWeight = float3(0.2126f, 0.7152f, 0.0722f);

// 中間の灰色 (log2(0.18))
static const float kMiddleGrayLog = -2.47393f;

/// <summary>暗い所ほど色が抜けて青っぽく見せる (プルキンエ現象)</summary>
/// <param name="luminance">自動露出前の明るさ</param>
float3 ApplyPurkinje(float3 hdr, float luminance)
{
    // 桿体の感度の近似と、そのとき感じる色 (明るさ1に正規化)
    static const float3 kRodWeight = float3(0.05f, 0.56f, 0.39f);
    static const float3 kNightColor = float3(0.74f, 1.01f, 1.68f);

    float night = purkinje * (1.0f - smoothstep(0.0f, purkinjeLuminance, luminance));
    return lerp(hdr, dot(hdr, kRodWeight) * kNightColor, night);
}

/// <summary>局所トーンマップ。まわりの明るさを中間へ寄せ、細部の明暗はそのまま残す</summary>
/// <remarks>明るさの近い点だけを混ぜて拾う</remarks>
float3 ApplyLocalTonemap(float3 hdr, float2 uv, float finalExposure)
{
    float luminance = dot(hdr, kLumaWeight);
    if (luminance <= 0.0f)
    {
        return hdr;
    }
    float logLuminance = log2(luminance);
    // 地図は露出前の画面から作っている
    float logExposure = log2(max(finalExposure, 1e-6f));

    uint2 size;
    gLocalTonemap.GetDimensions(size.x, size.y);
    float2 position = uv * float2(size) - 0.5f;
    int2 base = int2(floor(position));
    float2 f = position - float2(base);

    float sum = 0.0f;
    float weightSum = 0.0f;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        int2 offset = int2(i & 1, i >> 1);
        int2 texel = clamp(base + offset, int2(0, 0), int2(size) - 1);
        float value = gLocalTonemap.Load(int3(texel, 0)) + logExposure;
        float bilinear = (offset.x != 0 ? f.x : 1.0f - f.x) * (offset.y != 0 ? f.y : 1.0f - f.y);
        // 1段 (1EV) 離れると重みが半分
        float difference = value - logLuminance;
        float weight = bilinear * (exp2(-difference * difference) + 1e-3f);
        sum += value * weight;
        weightSum += weight;
    }
    float surroundLog = sum / max(weightSum, 1e-6f);

    float targetLog = lerp(surroundLog, kMiddleGrayLog, localTonemap) + (logLuminance - surroundLog) * localTonemapDetail;
    return hdr * exp2(targetLog - logLuminance);
}

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

// AgX (Troy Sobotka) の近似版。明るくなるほど白へ抜くので、強い赤がオレンジへ転びにくい。
// 素のままだとコントラストが低めに出る
float3 AgX(float3 x)
{
    // 入力側の行列。原色を少し内側へ寄せてからカーブへ通す
    float3 v = float3(
        dot(float3(0.842479062253094f, 0.0784335999999992f, 0.0792237451477643f), x),
        dot(float3(0.0423282422610123f, 0.878468636469772f, 0.0791661274605434f), x),
        dot(float3(0.0423756549057051f, 0.0784336f, 0.879142973793104f), x));

    // 明るさを対数で持つ。扱う範囲は中間グレーの上下あわせて約16.5段
    const float minEv = -12.47393f;
    const float maxEv = 4.026069f;
    v = clamp(log2(max(v, 1e-10f)), minEv, maxEv);
    v = (v - minEv) / (maxEv - minEv);

    // S字カーブの多項式近似
    float3 v2 = v * v;
    float3 v4 = v2 * v2;
    v = 15.5f * v4 * v2 - 40.14f * v4 * v + 31.96f * v4 - 6.868f * v2 * v + 0.4298f * v2 + 0.1191f * v - 0.00232f;

    // 出力側の行列(入力側の逆)
    v = float3(
        dot(float3(1.19687900512017f, -0.0980208811401368f, -0.0990297440797205f), v),
        dot(float3(-0.0528968517574562f, 1.15190312990417f, -0.0989611768448433f), v),
        dot(float3(-0.0529716355144438f, -0.0980434501171241f, 1.15107367264116f), v));

    // カーブの出力は表示用(ガンマ後)の値。出力先のRTVが _SRGB なのでリニアへ戻して返す
    return pow(saturate(v), 2.2f);
}

/// <summary>その位置の、画面へ出す色(表示レンジのリニア)</summary>
/// <remarks>ギザギザをならす処理は「見た目の明るさ」で輪郭を探すので、周りの点もここを通して比べる</remarks>
float3 Shade(float2 uv, float2 texelSize)
{
    // サンプラがWRAPなので、端の半テクセルが反対側を拾わないよう内側へ寄せる
    uv = clamp(uv, 0.5f * texelSize, 1.0f - 0.5f * texelSize);

    float finalExposure = (autoExposure != 0) ? exposure * gAutoExposure[0] : exposure;
    float3 hdr;
    if (chromaticAberration > 0.0f)
    {
        // 色収差。赤は外へ・青は内へ。強さ1で角が約6画素
        float2 offset = (uv - 0.5f) * (chromaticAberration * 0.01f);
        hdr.r = g_InputTexture.SampleLevel(g_Sampler, clamp(uv + offset, 0.5f * texelSize, 1.0f - 0.5f * texelSize), 0.0f).r;
        hdr.g = g_InputTexture.SampleLevel(g_Sampler, uv, 0.0f).g;
        hdr.b = g_InputTexture.SampleLevel(g_Sampler, clamp(uv - offset, 0.5f * texelSize, 1.0f - 0.5f * texelSize), 0.0f).b;
    }
    else
    {
        hdr = g_InputTexture.SampleLevel(g_Sampler, uv, 0.0f).rgb;
    }
    // 色温度。トーンマップの前に掛ける
    hdr *= whiteBalance * finalExposure;

    if (purkinje > 0.0f)
    {
        // 自動露出の後でも暗い場面は青く寄せる
        float manualExposureRatio = (finalExposure > 0.0f) ? exposure / finalExposure : 1.0f;
        hdr = ApplyPurkinje(hdr, dot(hdr, kLumaWeight) * manualExposureRatio);
    }
    if (localTonemap > 0.0f)
    {
        hdr = ApplyLocalTonemap(hdr, uv, finalExposure);
    }

    float3 mapped;
    switch (tonemapMode)
    {
        case 1:
            mapped = Reinhard(hdr);
            break;
        case 2:
            mapped = ACESFilm(hdr);
            break;
        case 3:
            mapped = AgX(hdr);
            break;
        default:
            mapped = hdr; // 素通し。HDR化の切り分け用
            break;
    }
    mapped = saturate(mapped);

    // 既定値(どちらも1)のときは一切触らない
    if (saturation != 1.0f || contrast != 1.0f)
    {
        mapped = max(lerp(dot(mapped, kLumaWeight), mapped, saturation), 0.0f);
        // 中間の明るさを軸にするため、見た目の明るさ(ガンマ後)で掛ける
        float3 perceptual = pow(mapped, 1.0f / 2.2f);
        perceptual = saturate((perceptual - 0.5f) * contrast + 0.5f);
        mapped = pow(perceptual, 2.2f);
    }

    // 影と光に別々の色味を乗せる。どちらも白なら一切触らない
    if (any(shadowTint != 1.0f) || any(highlightTint != 1.0f))
    {
        float brightness = sqrt(saturate(dot(mapped, kLumaWeight)));
        mapped = saturate(mapped * lerp(shadowTint, highlightTint, smoothstep(0.0f, 1.0f, brightness)));
    }

    return mapped;
}

/// <summary>見た目の明るさ。リニアのままだと暗い側の輪郭を見落とす</summary>
float PerceptualLuma(float3 color)
{
    return sqrt(dot(color, kLumaWeight));
}

/// <summary>輪郭のギザギザをならす(FXAA の簡易版)</summary>
/// <remarks>斜めの4点と明るさを比べ、輪郭があればその向きに沿って数点を混ぜる。平らな所は何もしない</remarks>
float3 ApplyFXAA(float2 uv, float2 texelSize, float3 center)
{
    float3 nw = Shade(uv + float2(-1.0f, -1.0f) * texelSize, texelSize);
    float3 ne = Shade(uv + float2(1.0f, -1.0f) * texelSize, texelSize);
    float3 sw = Shade(uv + float2(-1.0f, 1.0f) * texelSize, texelSize);
    float3 se = Shade(uv + float2(1.0f, 1.0f) * texelSize, texelSize);

    float lumaNW = PerceptualLuma(nw);
    float lumaNE = PerceptualLuma(ne);
    float lumaSW = PerceptualLuma(sw);
    float lumaSE = PerceptualLuma(se);
    float lumaM = PerceptualLuma(center);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // 明るさの差が小さい所は輪郭ではない。ここで抜けるので、大半の画素は4点を引くだけで終わる
    if (lumaMax - lumaMin < max(0.0312f, lumaMax * 0.125f))
    {
        return center;
    }

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25f / 8.0f), 1.0f / 128.0f);
    float rcpDirMin = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, -8.0f, 8.0f) * texelSize;

    float3 rgbA = 0.5f * (Shade(uv + dir * (1.0f / 3.0f - 0.5f), texelSize) +
                          Shade(uv + dir * (2.0f / 3.0f - 0.5f), texelSize));
    float3 rgbB = rgbA * 0.5f + 0.25f * (Shade(uv + dir * -0.5f, texelSize) +
                                           Shade(uv + dir * 0.5f, texelSize));

    // 広く混ぜた方が周りの明るさの範囲を外れたら、輪郭をまたいで別の面を拾っている
    float lumaB = PerceptualLuma(rgbB);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

/// <summary>輪郭をくっきりさせる。周りの平均との差を上乗せする</summary>
/// <remarks>FXAA の後に掛ける。周りの明るさの範囲を超えないよう抑えるので縁取りは出ない</remarks>
float3 ApplySharpen(float2 uv, float2 texelSize, float3 center)
{
    float3 n = Shade(uv + float2(0.0f, -1.0f) * texelSize, texelSize);
    float3 s = Shade(uv + float2(0.0f, 1.0f) * texelSize, texelSize);
    float3 w = Shade(uv + float2(-1.0f, 0.0f) * texelSize, texelSize);
    float3 e = Shade(uv + float2(1.0f, 0.0f) * texelSize, texelSize);

    float3 lo = min(center, min(min(n, s), min(w, e)));
    float3 hi = max(center, max(max(n, s), max(w, e)));
    float3 sharpened = center + (center - (n + s + w + e) * 0.25f) * sharpen;
    return clamp(sharpened, lo, hi);
}

/// <summary>画面の四隅を暗くして、視線を中央へ寄せる</summary>
float3 ApplyVignette(float3 color, float2 uv)
{
    // 中央で0、辺の中点で1、角で約1.4
    float distance = length((uv - 0.5f) * 2.0f);
    return color * (1.0f - vignette * smoothstep(0.5f, 1.4f, distance));
}

/// <summary>8bitへ落とすときに出る縞を、1段ぶんに満たない細かい模様で散らす</summary>
/// <remarks>段は見た目の明るさ(sRGB)で等間隔なので、そちらの尺度で足す。リニアのまま足すと暗い所で粒が目立つ</remarks>
float3 ApplyDither(float3 color, float2 pixel)
{
    float noise = frac(52.9829189f * frac(dot(pixel, float2(0.06711056f, 0.00583715f))));

    float3 perceptual = pow(color, 1.0f / 2.2f);
    perceptual += (noise - 0.5f) / 255.0f;
    return pow(saturate(perceptual), 2.2f);
}

/// <summary>褪せたフィルム。黒を浮かせ、白を少し抑える</summary>
float3 ApplyFade(float3 color)
{
    float3 perceptual = pow(max(color, 0.0f), 1.0f / 2.2f);
    perceptual = perceptual * (1.0f - 0.18f * fade) + 0.09f * fade;
    return pow(perceptual, 2.2f);
}

/// <summary>フィルムの粒子。中間の明るさで最も目立つ</summary>
float3 ApplyFilmGrain(float3 color, float2 pixel)
{
    float noise = frac(sin(dot(pixel + grainSeed * float2(17.13f, 31.71f), float2(12.9898f, 78.233f))) * 43758.5453f);
    float3 perceptual = pow(saturate(color), 1.0f / 2.2f);
    float luma = dot(perceptual, kLumaWeight);
    float amount = filmGrain * 0.12f * (1.0f - abs(luma * 2.0f - 1.0f) * 0.7f);
    perceptual += (noise - 0.5f) * amount;
    return pow(saturate(perceptual), 2.2f);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 size;
    g_InputTexture.GetDimensions(size.x, size.y);
    float2 texelSize = 1.0f / size;

    float3 color = Shade(input.texcoord, texelSize);

    if (enableFXAA != 0)
    {
        color = ApplyFXAA(input.texcoord, texelSize, color);
    }

    if (sharpen > 0.0f)
    {
        color = ApplySharpen(input.texcoord, texelSize, color);
    }

    if (vignette > 0.0f)
    {
        color = ApplyVignette(color, input.texcoord);
    }

    if (fade > 0.0f)
    {
        color = ApplyFade(color);
    }

    if (filmGrain > 0.0f)
    {
        color = ApplyFilmGrain(color, input.position.xy);
    }

    if (enableDither != 0)
    {
        color = ApplyDither(color, input.position.xy);
    }

    return float4(color, 1.0f);
}
