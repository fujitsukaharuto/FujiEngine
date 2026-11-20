#pragma once
#include <cstdint>

namespace Graphics {

	/// <summary>
	/// 前方描画の PS (Object3d.PS) に入れる機能。フレームごとに、使う機能だけでコンパイルした PSO で描く
	/// </summary>
	/// <remarks>HLSL 側 (Object/ObjectFeatures.hlsli) と同じ値であること</remarks>
	enum ObjectShaderFeature : uint32_t {
		kObjectFeatureWetness = 1u << 0,			// 濡れた路面
		kObjectFeatureProbeGI = 1u << 1,			// DDGI
		kObjectFeatureBakedGI = 1u << 2,			// 焼いた間接光
		kObjectFeatureScreenGI = 1u << 3,			// 画面のプローブ・画面の照り返し
		kObjectFeatureInlineAO = 1u << 4,			// PS でレイを飛ばす AO
		kObjectFeatureTranslucentShadow = 1u << 5,	// 半透明の色つきの影
		kObjectFeaturePointLights = 1u << 6,		// 影ありの点光源
		kObjectFeatureSpotLights = 1u << 7,			// 影ありのスポット
		kObjectFeatureClusteredLights = 1u << 8,	// 影なしのライト
		kObjectFeatureShapedLights = 1u << 9,		// 影なしのうち棒と長方形
		kObjectFeatureReflectionTrace = 1u << 10,	// 映り込みのレイ
		kObjectFeatureReflectionObjects = 1u << 11,	// 映り込みに物の色を出す
		kObjectFeatureScreenReflection = 1u << 12,	// 別パスの映り込み
		kObjectFeatureReflectionProbes = 1u << 13,	// 置いた反射プローブ
		kObjectFeatureLocalFog = 1u << 14,			// 局所の霧
		kObjectFeatureVolumetricFog = 1u << 15,		// ボリュメトリックフォグの表
		kObjectFeatureRefraction = 1u << 16,		// 半透明の屈折
		kObjectFeatureSimpleLighting = 1u << 17,
		kObjectFeaturePBRLighting = 1u << 18,
		kObjectFeatureCloudShadow = 1u << 19,		// 雲の影
		kObjectFeatureSoftLocalShadow = 1u << 20,	// 点光源とスポットの柔らかい影
		kObjectFeatureAll = (1u << 21) - 1,
	};

}
