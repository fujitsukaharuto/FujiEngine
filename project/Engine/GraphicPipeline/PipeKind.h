#pragma once

/// <summary>
/// パイプラインの種類
/// </summary>
enum class Pipe : int {

	None,
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
	MetaBall,
	ShockWave,
	Fire,
	Thunder,
	CRT,
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
	TrailEmitCS,
	InitArgsCS,
	AliveCountCS,

	Count
};