#pragma once
#include "Engine/Graphics/Pipeline/RootParam.h"


namespace Graphics {

	/// <summary>
	/// シェーダのルートパラメータ名
	/// </summary>
	/// <remarks>文字列は HLSL 側の宣言と一致していること。直書きせずここの定数を使う</remarks>
	namespace RootName {

		// --- オブジェクト描画共通 ---
		inline const RootParam kTransformationMatrix{ "gTransformationMatrix" };
		inline const RootParam kTransformationMatries{ "gTransformationMatries" };	// パーティクル用(HLSL側の綴りに合わせている)
		inline const RootParam kCamera{ "gCamera" };
		inline const RootParam kCameraInfo{ "CameraInfo" };
		inline const RootParam kMaterial{ "gMaterial" };
		inline const RootParam kTexture{ "gTexture" };
		inline const RootParam kTextures{ "gTextures" };
		inline const RootParam kEnvironment{ "gEnvironment" };
		inline const RootParam kLights{ "gLights" };
		inline const RootParam kObjIDData{ "ObjIDData" };
		inline const RootParam kSceneTLAS{ "gSceneTLAS" };			// レイトレの加速構造(TLAS)
		inline const RootParam kRayInstances{ "gRayInstances" };	// レイが当たった物の見た目(反射に映す色)
		inline const RootParam kRayVertices{ "gRayVertices" };		// レイが当たった点の UV と法線を引くための、全メッシュの頂点
		inline const RootParam kRayIndices{ "gRayIndices" };		// 同、インデックス
		inline const RootParam kRayTextures{ "gRayTextures" };		// 抜きのある物の影が引くテクスチャ

		// --- 焼いた間接光 (Common/BakedGI.hlsli) ---
		inline const RootParam kSkyCube{ "gSkyCube" };				// 雲まで描き込んだ空

		inline const RootParam kBakedGI{ "gBakedGI" };
		inline const RootParam kBakedSky{ "gBakedSky" };
		inline const RootParam kBakedSun{ "gBakedSun" };
		inline const RootParam kBakedDistance{ "gBakedDistance" };
		inline const RootParam kBakedProbeData{ "gBakedProbeData" };
		inline const RootParam kAOTexture{ "gAOTexture" };			// 画面空間の計算済みAO(前方描画が引く)
		inline const RootParam kShadowTexture{ "gShadowTexture" };	// 画面空間の計算済み遮蔽率(前方描画が引く)
		inline const RootParam kIrradianceMap{ "gIrradianceMap" };	// 焼いた放射照度(拡散のアンビエント)
		inline const RootParam kPrefilteredEnv{ "gPrefilteredEnv" };	// 粗さごとに焼いた環境マップ(鏡面のアンビエント)
		inline const RootParam kBRDFLut{ "gBRDFLut" };				// split-sum の第2項の数表

		// --- IBL を焼く側 ---
		inline const RootParam kSourceEnv{ "gSourceEnv" };
		inline const RootParam kIrradianceOut{ "gIrradianceOut" };
		inline const RootParam kPrefilteredOut{ "gPrefilteredOut" };
		inline const RootParam kIrradianceParam{ "IrradianceParam" };
		inline const RootParam kPrefilterParam{ "PrefilterParam" };
		inline const RootParam kBRDFLutOut{ "gBRDFLutOut" };
		inline const RootParam kPreviewSource{ "gPreviewSource" };
		inline const RootParam kPreviewOut{ "gPreviewOut" };
		inline const RootParam kPreviewParam{ "PreviewParam" };

		// --- 計算する空 ---
		inline const RootParam kAtmosphereParams{ "AtmosphereParams" };
		inline const RootParam kTransmittanceLut{ "gTransmittanceLut" };
		inline const RootParam kTransmittanceOut{ "gTransmittanceOut" };
		inline const RootParam kMultiScatterLut{ "gMultiScatterLut" };
		inline const RootParam kMultiScatterOut{ "gMultiScatterOut" };
		inline const RootParam kSkyViewLut{ "gSkyViewLut" };			// 空の描画も引く
		inline const RootParam kSkyViewOut{ "gSkyViewOut" };
		inline const RootParam kSkyCubeOut{ "gSkyCubeOut" };
		inline const RootParam kSummaryOut{ "gSummaryOut" };

		// --- プローブの間接光 (DDGI) ---
		inline const RootParam kDDGIVolume{ "gDDGIVolume" };			// 更新している格子 1つぶん
		inline const RootParam kDDGIVolumes{ "gDDGIVolumes" };			// 引く側が使う全部の格子
		inline const RootParam kDDGIIrradiance{ "gDDGIIrradiance" };	// 前方描画も引く
		inline const RootParam kDDGIDistance{ "gDDGIDistance" };		// 前方描画も引く
		inline const RootParam kDDGIRayData{ "gDDGIRayData" };			// 飛ばす側は UAV、畳み込む側は SRV
		inline const RootParam kDDGIIrradianceOut{ "gDDGIIrradianceOut" };
		inline const RootParam kDDGIDistanceOut{ "gDDGIDistanceOut" };
		inline const RootParam kDDGIProbeData{ "gDDGIProbeData" };		// 前方描画も引く
		inline const RootParam kDDGIProbeDataOut{ "gDDGIProbeDataOut" };

		// --- G-Buffer / 画面空間レイトレ ---
		inline const RootParam kGBufferNormal{ "gGBufferNormal" };
		inline const RootParam kGBufferDepth{ "gGBufferDepth" };
		inline const RootParam kGBufferNormalPrev{ "gGBufferNormalPrev" };
		inline const RootParam kGBufferDepthPrev{ "gGBufferDepthPrev" };
		inline const RootParam kAOOutput{ "gAOOutput" };
		inline const RootParam kAOPassParam{ "AOPassParam" };
		inline const RootParam kShadowOutput{ "gShadowOutput" };
		inline const RootParam kShadowPassParam{ "ShadowPassParam" };

		// --- 画面のプローブの間接光 ---
		inline const RootParam kScreenProbeParam{ "ScreenProbeParam" };
		inline const RootParam kProbeSH{ "gProbeSH" };
		inline const RootParam kProbeSHOut{ "gProbeSHOut" };
		inline const RootParam kProbeSHFiltered{ "gProbeSHFiltered" };
		inline const RootParam kProbeSHFilteredOut{ "gProbeSHFilteredOut" };
		inline const RootParam kProbePosition{ "gProbePosition" };
		inline const RootParam kProbePositionOut{ "gProbePositionOut" };
		inline const RootParam kProbeNormal{ "gProbeNormal" };
		inline const RootParam kProbeNormalOut{ "gProbeNormalOut" };
		inline const RootParam kScreenGIRaw{ "gScreenGIRaw" };
		inline const RootParam kScreenGIRawOut{ "gScreenGIRawOut" };
		inline const RootParam kScreenGIHistoryPrev{ "gScreenGIHistoryPrev" };
		inline const RootParam kScreenGICountPrev{ "gScreenGICountPrev" };
		inline const RootParam kScreenGIOut{ "gScreenGIOut" };
		inline const RootParam kScreenGICountOut{ "gScreenGICountOut" };
		inline const RootParam kScreenGI{ "gScreenGI" };				// 前方描画が引く結果

		// --- 画面の映り込み ---
		inline const RootParam kReflectionPassParam{ "ReflectionPassParam" };
		inline const RootParam kReflectionTrace{ "gReflectionTrace" };
		inline const RootParam kReflectionTraceOut{ "gReflectionTraceOut" };
		inline const RootParam kReflectionBlurInput{ "gReflectionBlurInput" };
		inline const RootParam kReflectionOut{ "gReflectionOut" };
		inline const RootParam kReflectionSceneColor{ "gReflectionSceneColor" };
		inline const RootParam kScreenReflection{ "gScreenReflection" };	// 前方描画が引く結果

		// --- 反射プローブ (ReflectionProbePass / Common/ReflectionProbe.hlsli) ---
		inline const RootParam kReflectionProbes{ "gReflectionProbes" };			// 畳んだキューブの配列
		inline const RootParam kReflectionProbeParams{ "gReflectionProbeParams" };
		inline const RootParam kReflectionProbeCaptureParam{ "ReflectionProbeCaptureParam" };
		inline const RootParam kProbeCaptureOut{ "gProbeCaptureOut" };
		inline const RootParam kDownsampleSource{ "gDownsampleSource" };
		inline const RootParam kDownsampleOut{ "gDownsampleOut" };
		inline const RootParam kProbeCapture{ "gProbeCapture" };
		inline const RootParam kProbePrefilteredOut{ "gProbePrefilteredOut" };
		inline const RootParam kProbePrefilterParam{ "ProbePrefilterParam" };

		// --- 雨 (RainRenderer) ---
		inline const RootParam kRainParams{ "RainParams" };

		// --- 半透明の屈折 (RefractionCapture) ---
		inline const RootParam kRefractionScene{ "gRefractionScene" };
		inline const RootParam kRefractionParams{ "RefractionParams" };

		// --- 影なしのライト (ClusteredLightPass) ---
		inline const RootParam kClusterParams{ "gClusterParams" };
		inline const RootParam kClusteredLights{ "gClusteredLights" };
		inline const RootParam kClusterCountsOut{ "gClusterCountsOut" };
		inline const RootParam kClusterIndicesOut{ "gClusterIndicesOut" };
		inline const RootParam kClusterCounts{ "gClusterCounts" };		// 前方描画が引く側
		inline const RootParam kClusterIndices{ "gClusterIndices" };

		// --- 光の筋 (VolumetricLightPass) ---
		inline const RootParam kVolumetricParam{ "VolumetricParam" };
		inline const RootParam kVolumetricTraceOut{ "gVolumetricTraceOut" };
		inline const RootParam kVolumetricBlurInput{ "gVolumetricBlurInput" };
		inline const RootParam kVolumetricOut{ "gVolumetricOut" };
		inline const RootParam kVolumetricLight{ "gVolumetricLight" };	// 画面へ足す側が引く結果

		// --- デノイザ(AOと影で共用) ---
		inline const RootParam kDenoiseRaw{ "gDenoiseRaw" };
		inline const RootParam kDenoiseHistoryPrev{ "gDenoiseHistoryPrev" };
		inline const RootParam kDenoiseHistoryOut{ "gDenoiseHistoryOut" };
		inline const RootParam kDenoiseFilterInput{ "gDenoiseFilterInput" };
		inline const RootParam kDenoiseFilterOut{ "gDenoiseFilterOut" };
		inline const RootParam kDenoiseTemporalParam{ "DenoiseTemporalParam" };
		inline const RootParam kDenoiseSpatialParam{ "DenoiseSpatialParam" };

		// --- ピッキング ---
		inline const RootParam kPickingData{ "PickingData" };
		inline const RootParam kPickingBuffer{ "gPickingBuffer" };

		// --- ポストエフェクト ---
		inline const RootParam kOutputTexture{ "outputTexture" };
		inline const RootParam kInputTexture{ "InputTexture" };
		inline const RootParam kG_InputTexture{ "g_InputTexture" };					// 上の InputTexture とは別名(HLSL側で綴りが揃っていない)
		inline const RootParam kDepthTexture{ "gDepthTexture" };
		inline const RootParam kConstants{ "Constants" };
		inline const RootParam kBloomParams{ "BloomParams" };
		inline const RootParam kAutoExposureParams{ "AutoExposureParams" };
		inline const RootParam kAutoExposureHistogram{ "gHistogram" };
		inline const RootParam kAutoExposureOut{ "gExposureOut" };
		inline const RootParam kAutoExposure{ "gAutoExposure" };		// トーンマップが引く倍率
		inline const RootParam kLocalTonemap{ "gLocalTonemap" };		// トーンマップが引くまわりの明るさ
		inline const RootParam kBufferVisualizeParams{ "BufferVisualizeParams" };
		inline const RootParam kBloomPassParam{ "BloomPassParam" };
		inline const RootParam kBloomSource{ "gBloomSource" };
		inline const RootParam kBloomLow{ "gBloomLow" };
		inline const RootParam kBloomHigh{ "gBloomHigh" };
		inline const RootParam kBloomOutput{ "gBloomOutput" };
		inline const RootParam kBlurConstantBuffer{ "BlurConstantBuffer" };
		inline const RootParam kGrayscaleConstantBuffer{ "GrayscaleConstantBuffer" };
		inline const RootParam kVignetteConstantBuffer{ "VignetteConstantBuffer" };
		inline const RootParam kTonemapParams{ "TonemapParams" };
		inline const RootParam kGodRayParams{ "GodRayParams" };
		inline const RootParam kDepthOfFieldParams{ "DepthOfFieldParams" };
		inline const RootParam kMotionBlurParams{ "MotionBlurParams" };
		inline const RootParam kLensFlareParams{ "LensFlareParams" };
		inline const RootParam kVolumetricCompositeParams{ "VolumetricCompositeParams" };
		inline const RootParam kFog{ "gFog" };

		// --- GPUパーティクル: プール ---
		inline const RootParam kParticles_Trans{ "gParticles_Trans" };
		inline const RootParam kParticles_TransPrev{ "gParticles_TransPrev" };
		inline const RootParam kParticles_Scale{ "gParticles_Scale" };
		inline const RootParam kParticles_Color{ "gParticles_Color" };
		inline const RootParam kParticles_ColorPrev{ "gParticles_ColorPrev" };
		inline const RootParam kParticles_Time{ "gParticles_Time" };
		inline const RootParam kParticles_Flags{ "gParticles_Flags" };
		inline const RootParam kParticles_Velocity{ "gParticles_Velocity" };

		// --- GPUパーティクル: フリーリスト/描画リスト ---
		inline const RootParam kFreeList{ "gFreeList" };
		inline const RootParam kFreeListIndex{ "gFreeListIndex" };
		inline const RootParam kFreeListTailIndex{ "gFreeListTailIndex" };
		inline const RootParam kDrawParticleIndex{ "gDrawParticleIndex" };
		inline const RootParam kDrawArgs{ "gDrawArgs" };

		// --- GPUパーティクル: エミッタ/描画 ---
		inline const RootParam kPerView{ "gPerView" };
		inline const RootParam kPerFrame{ "gPerFrame" };
		inline const RootParam kEmitter{ "gEmitter" };
		inline const RootParam kMaskTex{ "gMaskTex" };
		inline const RootParam kTriangleCDF{ "gTriangleCDF" };
		inline const RootParam kVertices{ "gVertices" };
		inline const RootParam kIndices{ "gIndices" };
		inline const RootParam kSceneDepth{ "gSceneDepth" };
		inline const RootParam kSplatAccum{ "gSplatAccum" };
		inline const RootParam kSplatParam{ "gSplatParam" };

	}

}
