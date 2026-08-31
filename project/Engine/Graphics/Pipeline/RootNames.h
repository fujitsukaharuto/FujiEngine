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

		// --- G-Buffer / 画面空間レイトレ ---
		inline const RootParam kGBufferNormal{ "gGBufferNormal" };
		inline const RootParam kGBufferDepth{ "gGBufferDepth" };
		inline const RootParam kGBufferNormalPrev{ "gGBufferNormalPrev" };
		inline const RootParam kGBufferDepthPrev{ "gGBufferDepthPrev" };
		inline const RootParam kAOOutput{ "gAOOutput" };
		inline const RootParam kAOPassParam{ "AOPassParam" };
		inline const RootParam kShadowOutput{ "gShadowOutput" };
		inline const RootParam kShadowPassParam{ "ShadowPassParam" };

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
		inline const RootParam kBlurConstantBuffer{ "BlurConstantBuffer" };
		inline const RootParam kGrayscaleConstantBuffer{ "GrayscaleConstantBuffer" };
		inline const RootParam kVignetteConstantBuffer{ "VignetteConstantBuffer" };
		inline const RootParam kTonemapParams{ "TonemapParams" };

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
