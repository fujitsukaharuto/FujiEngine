#pragma once
#include "Engine/Graphics/Pipeline/RootParam.h"


namespace Graphics {

	/// <summary>
	/// シェーダのルートパラメータ名。文字列は HLSL 側の宣言と一致していなければならないので、
	/// 呼び出し側にリテラルを直書きせずここの定数を使う(綴り違いをリンク時に潰せる)。
	/// </summary>
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

		// --- G-Buffer / 画面空間レイトレ ---
		inline const RootParam kGBufferNormal{ "gGBufferNormal" };
		inline const RootParam kGBufferDepth{ "gGBufferDepth" };
		inline const RootParam kGBufferNormalPrev{ "gGBufferNormalPrev" };
		inline const RootParam kGBufferDepthPrev{ "gGBufferDepthPrev" };
		inline const RootParam kAOOutput{ "gAOOutput" };
		inline const RootParam kAOPassParam{ "AOPassParam" };

		// --- AOのデノイザ ---
		inline const RootParam kAORaw{ "gAORaw" };
		inline const RootParam kAOHistoryPrev{ "gAOHistoryPrev" };
		inline const RootParam kAOHistoryOut{ "gAOHistoryOut" };
		inline const RootParam kAOFilterInput{ "gAOFilterInput" };
		inline const RootParam kAOFilterOut{ "gAOFilterOut" };
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
