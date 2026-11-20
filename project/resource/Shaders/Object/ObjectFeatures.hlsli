#ifndef OBJECT_FEATURES_HLSLI
#define OBJECT_FEATURES_HLSLI

// 前方描画の PS に入れる機能。C++ 側 (ObjectShaderFeatures.h) と同じ値であること
static const uint kObjectFeatureWetness = 1u << 0;            // 濡れた路面
static const uint kObjectFeatureProbeGI = 1u << 1;            // DDGI
static const uint kObjectFeatureBakedGI = 1u << 2;            // 焼いた間接光
static const uint kObjectFeatureScreenGI = 1u << 3;           // 画面のプローブ・画面の照り返し
static const uint kObjectFeatureInlineAO = 1u << 4;           // PS でレイを飛ばす AO
static const uint kObjectFeatureTranslucentShadow = 1u << 5;  // 半透明の色つきの影
static const uint kObjectFeaturePointLights = 1u << 6;        // 影ありの点光源
static const uint kObjectFeatureSpotLights = 1u << 7;         // 影ありのスポット
static const uint kObjectFeatureClusteredLights = 1u << 8;    // 影なしのライト
static const uint kObjectFeatureShapedLights = 1u << 9;       // 影なしのうち棒と長方形
static const uint kObjectFeatureReflectionTrace = 1u << 10;   // 映り込みのレイ
static const uint kObjectFeatureReflectionObjects = 1u << 11; // 映り込みに物の色を出す
static const uint kObjectFeatureScreenReflection = 1u << 12;  // 別パスの映り込み
static const uint kObjectFeatureReflectionProbes = 1u << 13;  // 置いた反射プローブ
static const uint kObjectFeatureLocalFog = 1u << 14;          // 局所の霧
static const uint kObjectFeatureVolumetricFog = 1u << 15;     // ボリュメトリックフォグの表
static const uint kObjectFeatureRefraction = 1u << 16;        // 半透明の屈折
static const uint kObjectFeatureSimpleLighting = 1u << 17;
static const uint kObjectFeaturePBRLighting = 1u << 18;
static const uint kObjectFeatureCloudShadow = 1u << 19;       // 雲の影
static const uint kObjectFeatureSoftLocalShadow = 1u << 20;   // 点光源とスポットの柔らかい影

// 付けずにコンパイルしたもの (ルートシグネチャを作る側) は全部入り
#ifndef OBJECT_FEATURES
#define OBJECT_FEATURES 0xFFFFFFFF
#endif
static const uint kObjectFeatures = OBJECT_FEATURES;

/// <summary>この PSO にその機能のコードがあるか。無ければ分岐ごと消える</summary>
bool HasObjectFeature(uint feature)
{
    return (kObjectFeatures & feature) != 0;
}

#endif // OBJECT_FEATURES_HLSLI
