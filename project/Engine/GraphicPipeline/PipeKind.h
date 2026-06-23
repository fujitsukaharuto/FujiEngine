#pragma once

/// <summary>
/// パイプラインの種類
/// </summary>
enum class Pipe : int {

	None,
	GPUParticleSynthesis, // 1/4解像度GPUパーティクルRTをシーンへ加算合成するフルスクリーンパイプ
	Normal,
	NormalAdd,
	NormalNode,
	NormalNodeAdd,
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