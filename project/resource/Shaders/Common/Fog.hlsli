#ifndef FOG_HLSLI
#define FOG_HLSLI

// 空気(フォグ)。高さで薄くなる指数フォグで、通り抜ける量を式で直接求める。
// 設定は配置データの環境に持つ。濃さが0なら何もしない

// LightManager.h の FogData と並びが一致していること
struct FogParams
{
    float3 color;         // フォグの色。horizonColor の平均
    float density;        // 濃さ。基準の高さで、1進むごとにこの割合ぶん霞む。0で無効
    float heightFalloff;  // 高さによる薄まり方。0なら高さに依らず一様
    float baseHeight;     // 濃さの基準にする高さ
    float startDistance;  // カメラからこの距離までは霞ませない
    float maxOpacity;     // どれだけ遠くてもこれ以上は霞ませない
    float3 cameraPosition; // カメラの位置。自前でカメラの定数を持たないシェーダ(空・粒子)用
    float skyAmount;      // 空へどれだけ掛けるか
    // 水平の向きごとのフォグの色。+X, -X, +Z, -Z の順。
    // 空から色を取る設定なら、その向きの空の地平線の色が入る。取らない設定なら4つとも color と同じ
    float4 horizonColor[4];
    float3 sunDirection;  // 太陽へ向かう単位ベクトル
    float sunScatter;     // 太陽の方向のフォグを明るくする強さ。0で無効
    float3 sunColor;      // 0番の平行光源の色と強さ
    float sunScatterPower; // 大きいほど太陽のまわりだけが明るくなる
    uint skyMode;         // Atmosphere.hlsli の kSkyMode*
    float skyIntensity;   // 計算する空の明るさ
    float2 skyPad;
};

/// <summary>太陽の方向を向いたときにフォグへ足す色</summary>
/// <param name="direction">視線の向き。正規化していなくてよい</param>
float3 FogSunGlow(FogParams fog, float3 direction)
{
    if (fog.sunScatter <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float facing = saturate(dot(normalize(direction), fog.sunDirection));
    return fog.sunColor * (fog.sunScatter * pow(facing, fog.sunScatterPower));
}

/// <summary>その向きのフォグの色。空の明るい側を向けば明るく、暗い側なら暗い</summary>
/// <param name="direction">視線の向き。正規化していなくてよい</param>
float3 FogColor(FogParams fog, float3 direction)
{
    float3 sunGlow = FogSunGlow(fog, direction);
    float2 h = direction.xz;
    float lengthSq = dot(h, h);
    // 真上・真下を向いているときは向きが決まらないので平均の色
    if (lengthSq < 1e-8f)
    {
        return fog.color + sunGlow;
    }

    // 向きの成分の2乗は足すと1になるので、そのまま混ぜる割合に使える
    float2 weight = h * h / lengthSq;
    float3 colorX = (h.x >= 0.0f) ? fog.horizonColor[0].rgb : fog.horizonColor[1].rgb;
    float3 colorZ = (h.y >= 0.0f) ? fog.horizonColor[2].rgb : fog.horizonColor[3].rgb;
    return colorX * weight.x + colorZ * weight.y + sunGlow;
}

/// <summary>視線がフォグの中を通り抜ける量</summary>
/// <param name="originY">視点の高さ</param>
/// <param name="dirY">視線の向き(単位ベクトル)の上下成分</param>
/// <param name="distance">視点から対象までの距離</param>
float FogOpticalDepth(FogParams fog, float originY, float dirY, float distance)
{
    // 手前の霞ませない区間を飛ばし、そこから先だけを数える
    float skipped = min(distance, fog.startDistance);
    float travel = distance - skipped;
    float startY = originY + dirY * skipped;

    float k = fog.heightFalloff;
    // 出発点の高さでの濃さ。基準より上なら薄く、下なら濃い
    float densityAtStart = fog.density * exp(clamp(-k * (startY - fog.baseHeight), -20.0f, 20.0f));

    // 進むにつれて高さが変わるぶんの補正。上へ向かうほど薄い所を通るので小さくなる
    float a = max(k * dirY * travel, -20.0f);
    float factor = (abs(a) > 1e-3f) ? (1.0f - exp(-a)) / a : 1.0f - 0.5f * a;

    return densityAtStart * travel * factor;
}

/// <summary>その点がどれだけ霞むか。0でそのまま、1で完全にフォグの色</summary>
float FogAmount(FogParams fog, float3 cameraPosition, float3 worldPosition)
{
    if (fog.density <= 0.0f)
    {
        return 0.0f;
    }

    float3 toPoint = worldPosition - cameraPosition;
    float distance = length(toPoint);
    if (distance < 1e-4f)
    {
        return 0.0f;
    }

    float depth = FogOpticalDepth(fog, cameraPosition.y, toPoint.y / distance, distance);
    return min(1.0f - exp(-depth), fog.maxOpacity);
}

// 空をこの距離にある物として霞ませる。far クリップと同じくらい。
// 無限遠にすると、ごく薄い設定でも空だけが完全にフォグの色になる
static const float kFogSkyDistance = 1000.0f;

/// <summary>空がどれだけ霞むか。向きだけで決まる</summary>
/// <remarks>地平線がいちばん濃く、見上げるほど薄くなる。地平線での濃さは、同じ距離にある地面とつながる</remarks>
/// <param name="direction">視線の向き(単位ベクトル)</param>
float FogAmountSky(FogParams fog, float3 direction)
{
    if (fog.density <= 0.0f)
    {
        return 0.0f;
    }

    float depth = FogOpticalDepth(fog, fog.cameraPosition.y, direction.y, kFogSkyDistance);
    float amount = min(1.0f - exp(-depth), fog.maxOpacity);

    // 高さで薄まらない設定だと、空のどの向きも同じだけ霞んで一面がフォグの色になる。
    // 見上げるほど薄くして、霞を地平線の帯に留める
    amount *= 1.0f - smoothstep(0.0f, 0.6f, direction.y);

    return amount * fog.skyAmount;
}

/// <summary>フォグを掛ける</summary>
/// <param name="fogColor">FogColor() で求めた、その向きのフォグの色</param>
/// <param name="additive">0以外なら加算合成で描かれる物。色を足す描き方なので、フォグの色へ寄せずに暗くするだけにする</param>
float3 ApplyFog(float3 color, float3 fogColor, float amount, float additive)
{
    return (additive != 0.0f) ? color * (1.0f - amount) : lerp(color, fogColor, amount);
}

#endif // FOG_HLSLI
