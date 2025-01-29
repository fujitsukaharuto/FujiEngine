#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"
#include "Audio/Audio.h"
#include "Particle/ParticleEmitter.h"

class PlayerJump : public BasePlayerBehavior {
private:
	enum class Step {
		JUMP,
		RETURNROOT,
	};
	enum class RenditionStep {
		SLOPE,
		SLOPEBACK,
	};
	/// ===================================================
	///private varians
	/// ===================================================
	RenditionStep renditionStep_;
	Step step_;

	float speed_;
	SoundData jumpSE_;
	ParticleEmitter jumpEmit_;

	float backStartTime_;

	// tempRotate
	float  slopeRotate_;
	float backRotate_;

	//easeT
	float footSlopeTime_;
	float footBackTime_;

public:
	//コンストラクタ
	PlayerJump(Player* boss);
	~PlayerJump();

	void Update()override;

	void Debug()override;

};
