#pragma once

namespace Graphics {

	/// <summary>
	/// パイプラインの種類
	/// </summary>
	enum class Pipe : int {

		None,
		GPUParticleSynthesis, // 1/4解像度GPUパーティクルRTをシーンへ加算合成するフルスクリーンパイプ
		DepthNormalPrepass, // 本描画の前に深度とワールド法線だけを書き出すG-Bufferパス
		RayTracedAOCS,        // G-Bufferを元に画面空間でAOを計算する
		RayTracedShadowCS,    // G-Bufferを元に画面空間で平行光源のソフトシャドウを計算する
		DenoiseTemporalCS,    // 前フレームへ再投影して信号を蓄積する(AOと影で共用)
		DenoiseSpatialCS,     // à-trous のエッジ保持ブラーを1回掛ける(AOと影で共用)
		IrradianceBakeCS,     // 環境マップを放射照度のキューブマップへ畳み込む(起動時に1回)
		PrefilterEnvCS,       // 環境マップを粗さごとに畳み込む(起動時に1回、ミップごとに1回)
		BRDFLutCS,            // split-sum の第2項の数表を焼く(起動時に1回)
		CubePreviewCS,        // キューブマップを正距円筒に開く(デバッグ表示用)
		Normal,
		NormalAdd,
		Sprite,
		Line3d,
		Particle,
		ParticleAlpha,
		ParticleSub,
		ParticleScreen,
		ParticleMultiply,
		ParticleSoftAdd,
		ParticlePreMulAlpha,
		ParticleCS,
		Animation,
		Skybox,
		BaseGrid,
		GrayCS,
		GaussCS,
		BoxFilterCS,
		RadialCS,
		VignetteCS,
		CRTCS,
		RetroTVCS,
		OutlineCS,
		LuminanceOutlineCS,
		BloomCS,
		RandomCS,
		SkinningCS,
		InitParticleCS,
		EmitParticleCS,
		EmitTexParticleCS,
		EmitSurfaceParticleCS,
		UpdateParticleCS,
		UpdateParticleSplatCS, // splatモード専用Update(カリング/描画リスト構築なしの軽量版)
		ClearParticleColorCS,  // プール2枚化(ピンポン): 書き込み先プールのcolorを0クリア
		TrailEmitCS,
		InitArgsCS,
		AliveCountCS,
		SplatClearCS,      // スプラット蓄積バッファのクリア(CS)
		SplatParticleCS,   // パーティクルを画面へ点描(CS)
		SplatComposite,    // 蓄積バッファをシーンへ加算合成(GFX)

		Count
	};

}
