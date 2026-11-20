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
    float cloudCoverage;  // 雲の量。0で雲を出さない
    float cloudTime;      // 雲を流す時刻(秒)
    // 空気遠近。カメラの高さの空気が一様に続くとみなす。係数は1mあたり
    float3 aerialRayleigh; // 太陽の光 × 透過率 × レイリー散乱
    float aerialMieG;      // ミー散乱の前方への偏り
    float3 aerialMie;      // 同、ミー散乱
    float aerialDistanceScale; // 0で空気遠近を掛けない
    float3 aerialAmbient;  // 多重散乱で回り込む光 × 散乱
    float starIntensity;   // 星の明るさ。0で描かない
    float3 aerialExtinction; // 散乱＋吸収
    float cloudShadow;     // 雲が太陽を遮る強さ (0〜1)
    float3 sunDiskColor;   // 大気を通った太陽の色
    float moonIntensity;   // 月の明るさ。0で描かない
    float3 skyGlowColor;   // 光害の色 × 明るさ。0で掛からない
    float skyGlowHeight;   // 光害が届く高さ (向きの y)
    // 2層目の霧 (UE の Second Fog Data)。色・始まる距離・上限は1層目と共有し、濃さは1層目に足す
    float secondDensity;       // 0で無効
    float secondHeightFalloff;
    float secondBaseHeight;    // ワールドの高さ
    float skyHorizonZenith;    // 計算する空で、カメラから見た地面の縁の天頂角 (SkyView の数表の並び)
};

/// <summary>霧が1層でも掛かる設定か</summary>
bool HasFog(FogParams fog)
{
    return fog.density > 0.0f || fog.secondDensity > 0.0f;
}

/// <summary>その高さの霧の濃さ (1mあたり)。2層の合計</summary>
float FogDensityAt(FogParams fog, float height)
{
    float first = fog.density * exp(clamp(-fog.heightFalloff * (height - fog.baseHeight), -20.0f, 20.0f));
    float second = fog.secondDensity * exp(clamp(-fog.secondHeightFalloff * (height - fog.secondBaseHeight), -20.0f, 20.0f));
    return first + second;
}

/// <summary>空気遠近で元の色に掛かる割合と、上に乗る空気の色</summary>
/// <remarks>3Dの表 (AerialPerspective.hlsli) を作っていないフレームに使う</remarks>
/// <param name="toPoint">カメラから対象までのベクトル</param>
/// <returns>空気遠近が掛かるか</returns>
bool AerialPerspectiveTerms(FogParams fog, float3 toPoint, out float3 transmittance, out float3 inScatter)
{
    transmittance = float3(1.0f, 1.0f, 1.0f);
    inScatter = float3(0.0f, 0.0f, 0.0f);
    if (fog.aerialDistanceScale <= 0.0f)
    {
        return false;
    }

    float distance = length(toPoint);
    if (distance < 1e-4f)
    {
        return false;
    }
    float3 direction = toPoint / distance;
    distance *= fog.aerialDistanceScale;

    float cosTheta = dot(direction, fog.sunDirection);
    float phaseRayleigh = 3.0f / (16.0f * 3.14159265359f) * (1.0f + cosTheta * cosTheta);
    float g = fog.aerialMieG;
    float g2 = g * g;
    float phaseMie = 3.0f / (8.0f * 3.14159265359f) * (1.0f - g2) / (2.0f + g2)
                     * (1.0f + cosTheta * cosTheta) / pow(max(1.0f + g2 - 2.0f * g * cosTheta, 1e-4f), 1.5f);

    float3 extinction = max(fog.aerialExtinction, 1e-9f);
    transmittance = exp(-extinction * distance);
    float3 source = fog.aerialRayleigh * phaseRayleigh + fog.aerialMie * phaseMie + fog.aerialAmbient;
    inScatter = source * (1.0f - transmittance) / extinction;
    return true;
}

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

/// <summary>高さで薄まる霧1層ぶんの、通り抜ける量</summary>
/// <param name="startY">数え始める点の高さ</param>
/// <param name="dirY">視線の向き(単位ベクトル)の上下成分</param>
/// <param name="travel">数える長さ</param>
float FogLayerOpticalDepth(float density, float falloff, float baseHeight, float startY, float dirY, float travel)
{
    if (density <= 0.0f)
    {
        return 0.0f;
    }
    // 出発点の高さでの濃さ。基準より上なら薄く、下なら濃い
    float densityAtStart = density * exp(clamp(-falloff * (startY - baseHeight), -20.0f, 20.0f));

    // 進むにつれて高さが変わるぶんの補正。上へ向かうほど薄い所を通るので小さくなる
    float a = max(falloff * dirY * travel, -20.0f);
    float factor = (abs(a) > 1e-3f) ? (1.0f - exp(-a)) / a : 1.0f - 0.5f * a;

    return densityAtStart * travel * factor;
}

/// <summary>視線がフォグの中を通り抜ける量。手前の excludeDistance までは数えない</summary>
/// <param name="originY">視点の高さ</param>
/// <param name="dirY">視線の向き(単位ベクトル)の上下成分</param>
/// <param name="distance">視点から対象までの距離</param>
float FogOpticalDepthFrom(FogParams fog, float originY, float dirY, float distance, float excludeDistance)
{
    float skipped = min(distance, excludeDistance);
    float travel = distance - skipped;
    float startY = originY + dirY * skipped;
    // 2層とも高さの指数なので、線に沿った積分もそれぞれの和になる
    return FogLayerOpticalDepth(fog.density, fog.heightFalloff, fog.baseHeight, startY, dirY, travel)
           + FogLayerOpticalDepth(fog.secondDensity, fog.secondHeightFalloff, fog.secondBaseHeight, startY, dirY, travel);
}

/// <summary>視線がフォグの中を通り抜ける量。手前の霞ませない区間を飛ばし、そこから先だけを数える</summary>
float FogOpticalDepth(FogParams fog, float originY, float dirY, float distance)
{
    return FogOpticalDepthFrom(fog, originY, dirY, distance, fog.startDistance);
}

/// <summary>その点がどれだけ霞むか。0でそのまま、1で完全にフォグの色</summary>
/// <param name="excludeDistance">この距離までは数えない。始まる距離と大きい方を使う (VolumetricFogExcludeDistance)</param>
float FogAmountFrom(FogParams fog, float3 cameraPosition, float3 worldPosition, float excludeDistance)
{
    if (!HasFog(fog))
    {
        return 0.0f;
    }

    float3 toPoint = worldPosition - cameraPosition;
    float distance = length(toPoint);
    if (distance < 1e-4f)
    {
        return 0.0f;
    }

    float depth = FogOpticalDepthFrom(fog, cameraPosition.y, toPoint.y / distance, distance, max(fog.startDistance, excludeDistance));
    return min(1.0f - exp(-depth), fog.maxOpacity);
}

/// <summary>その点がどれだけ霞むか。0でそのまま、1で完全にフォグの色</summary>
float FogAmount(FogParams fog, float3 cameraPosition, float3 worldPosition)
{
    return FogAmountFrom(fog, cameraPosition, worldPosition, 0.0f);
}

// 空をこの距離にある物として霞ませる。far クリップと同じくらい。
// 無限遠にすると、ごく薄い設定でも空だけが完全にフォグの色になる
static const float kFogSkyDistance = 1000.0f;

/// <summary>空がどれだけ霞むか。向きだけで決まる</summary>
/// <remarks>地平線がいちばん濃く、見上げるほど薄くなる。地平線での濃さは、同じ距離にある地面とつながる</remarks>
/// <param name="direction">視線の向き(単位ベクトル)</param>
/// <param name="excludeDistance">この距離までは数えない。始まる距離と大きい方を使う</param>
float FogAmountSkyFrom(FogParams fog, float3 direction, float excludeDistance)
{
    if (!HasFog(fog))
    {
        return 0.0f;
    }

    float depth = FogOpticalDepthFrom(fog, fog.cameraPosition.y, direction.y, kFogSkyDistance, max(fog.startDistance, excludeDistance));
    float amount = min(1.0f - exp(-depth), fog.maxOpacity);

    // 高さで薄まらない設定だと、空のどの向きも同じだけ霞んで一面がフォグの色になる。
    // 見上げるほど薄くして、霞を地平線の帯に留める
    amount *= 1.0f - smoothstep(0.0f, 0.6f, direction.y);

    return amount * fog.skyAmount;
}

/// <summary>空がどれだけ霞むか。向きだけで決まる</summary>
float FogAmountSky(FogParams fog, float3 direction)
{
    return FogAmountSkyFrom(fog, direction, 0.0f);
}

/// <summary>フォグを掛ける</summary>
/// <param name="fogColor">FogColor() で求めた、その向きのフォグの色</param>
/// <param name="additive">0以外なら加算合成で描かれる物。色を足す描き方なので、フォグの色へ寄せずに暗くするだけにする</param>
float3 ApplyFog(float3 color, float3 fogColor, float amount, float additive)
{
    return (additive != 0.0f) ? color * (1.0f - amount) : lerp(color, fogColor, amount);
}

#endif // FOG_HLSLI
