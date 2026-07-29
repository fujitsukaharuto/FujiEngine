struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint : TEXCOORD1;
};

cbuffer CameraInfo : register(b0)
{
    matrix invViewProj;
    float3 cameraPos;
    float pad;
    matrix viewProj;
};

struct PSOutput
{
    float4 color : SV_TARGET;
    float depth : SV_Depth;
};

// ---------------- 調整用パラメータ ----------------
static const float kBaseCell = 1.0; // 最小セルの一辺(ワールド単位)
static const float kCellPixels = 14.0; // セルが画面上でこのピクセル数を割ったら1段粗いLODへ
static const float kLineWidth = 1.0; // 格子線の太さ(ピクセル)
static const float kAxisWidth = 1.6; // 軸線の太さ(ピクセル)
static const float kMinRange = 60.0; // 最低限これだけの距離までは描く
static const float kRangeScale = 25.0; // カメラ高度の何倍まで描くか(引くほど遠くまで見える)
static const float kHorizonFalloff = 8.0; // 地平線付近のフェードの強さ

// 色はリニアHDRのオフスクリーンへ書く値で、画面に出るまでに「露光(既定2.0)を掛けて
// ACESトーンマップ」を通る。ACESはこの帯域を持ち上げるので、見た目の明るさは書いた値より
// かなり明るくなる(例: 0.25 と書くと画面では 0.616 になり、白飛びして見える)。
// そのため「画面でこう見せたい値」を逆算して入れてある。カッコ内が実際の表示値。
// 露光を既定から大きく動かすとこの前提が崩れるので、その時はここも振り直すこと
static const float3 kThinColor = float3(0.085, 0.085, 0.085); // (0.25)
static const float3 kThickColor = float3(0.142, 0.142, 0.142); // (0.42)
static const float3 kAxisXColor = float3(0.491, 0.076, 0.088); // z==0 を走る線 = X軸 (0.80,0.22,0.26)
static const float3 kAxisZColor = float3(0.076, 0.142, 0.491); // x==0 を走る線 = Z軸 (0.22,0.42,0.80)

// 線の中心からの距離(ピクセル)と線幅(ピクセル)から被覆率を返す。
// 1ピクセルを1辺1の正方形とみなした矩形フィルタ近似で、これがアンチエイリアスになる
float LineCoverage(float distPixel, float widthPixel)
{
    return saturate(widthPixel * 0.5 - distPixel + 0.5);
}

// セル間隔 cell の格子の被覆率。texel(1ピクセルが覆うワールド距離)で正規化するので、
// 線幅はカメラとの距離に関係なく常に widthPixel ピクセルになる
float GridCoverage(float2 posXZ, float2 texel, float cell, float widthPixel)
{
    float2 coord = posXZ / cell;
    float2 deriv = max(texel / cell, 1e-8);
    float2 distPixel = abs(frac(coord - 0.5) - 0.5) / deriv;
    return max(LineCoverage(distPixel.x, widthPixel),
               LineCoverage(distPixel.y, widthPixel));
}


PSOutput main(VSOutput input)
{
    PSOutput output;

    float3 rayDir = normalize(input.farPoint - input.nearPoint);
    // 視線が地面と平行だと交点が無限遠に飛ぶ
    if (abs(rayDir.y) < 1e-5)
    {
        discard;
    }
    
    // y=0 平面との交点。t<=0 は交点が視線の後ろ(＝地面を向いていない)
    // 符号で弾かないので、カメラが地面より下に潜っても描画される
    float t = -input.nearPoint.y / rayDir.y;
    if (t <= 0.0)
    {
        discard;
    }
    float3 worldPos = input.nearPoint + rayDir * t;

    // 1ピクセルが覆うワールド距離。斜めから見たときも正しく効くよう、
    // fwidth(=|ddx|+|ddy|)ではなく偏微分ベクトルの長さで取る
    float3 dpdx = ddx(worldPos);
    float3 dpdy = ddy(worldPos);
    float2 texel = float2(length(float2(dpdx.x, dpdy.x)),
                          length(float2(dpdx.z, dpdy.z)));
    
    // ---- LOD: セルが kCellPixels を割らないよう10倍刻みで粗くする ----
    float maxTexel = max(texel.x, texel.y);
    float lod = max(0.0, log10(maxTexel * kCellPixels / kBaseCell));
    float lodFade = frac(lod);

    float cell0 = kBaseCell * pow(10.0, floor(lod)); // 一番細かい格子
    float cell1 = cell0 * 10.0;
    float cell2 = cell1 * 10.0;

    float cov0 = GridCoverage(worldPos.xz, texel, cell0, kLineWidth);
    float cov1 = GridCoverage(worldPos.xz, texel, cell1, kLineWidth);
    float cov2 = GridCoverage(worldPos.xz, texel, cell2, kLineWidth);
    
    // 粗い格子ほど濃く出し、一番細かい格子は次の段へ移るにつれて消える。
    // これで「寄ると細かい線が湧き、引くと溶けて消える」Blenderの挙動になる
    float3 gridColor;
    float alpha;
    if (cov2 > 0.0)
    {
        gridColor = kThickColor;
        alpha = cov2;
    }
    else if (cov1 > 0.0)
    {
        gridColor = lerp(kThickColor, kThinColor, lodFade);
        alpha = cov1;
    }
    else
    {
        gridColor = kThinColor;
        alpha = cov0 * (1.0 - lodFade);
    }
    
    // ---- 軸線(格子と同じく常に一定のピクセル幅) ----
    float2 axisDist = abs(worldPos.xz) / max(texel, 1e-8);
    float axisZ = LineCoverage(axisDist.x, kAxisWidth); // x==0 を走る線 = Z軸
    float axisX = LineCoverage(axisDist.y, kAxisWidth); // z==0 を走る線 = X軸

    if (axisX > 0.0)
    {
        gridColor = kAxisXColor;
        alpha = max(alpha, axisX);
    }
    if (axisZ > 0.0)
    {
        gridColor = kAxisZColor;
        alpha = max(alpha, axisZ);
    }
    
    // ---- フェード ----
    // 地平線に近いほど格子が1ピクセルに潰れてジャギるので、そこで消す
    float horizonFade = saturate(abs(rayDir.y) * kHorizonFalloff);

    // 描画範囲はカメラ高度に追従させる(引くほど遠くまで見える=無限グリッドらしさ)
    float range = max(kMinRange, abs(cameraPos.y) * kRangeScale);
    float dist = distance(worldPos, cameraPos);
    float distFade = 1.0 - smoothstep(range * 0.4, range, dist);
    
    alpha *= horizonFade * distFade;

    if (alpha < 0.002)
    {
        discard;
    }

    // ワールド座標から深度を復元(3Dオブジェクトと正しく前後する)
    float4 clipPos = mul(float4(worldPos, 1.0), viewProj);

    output.depth = clipPos.z / clipPos.w;
    output.color = float4(gridColor, alpha);

    return output;
}