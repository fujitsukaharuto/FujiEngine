#pragma once

#include"BasePlayerState.h"
#include"math/Easing.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class PlayerReSpown : public BasePlayerState {
private:

	enum class Step {
	   RESPAWN,
	   GOROOT,
	};

	/// ===================================================
	///private varians
	/// ===================================================
	Step  step_;
	Easing ease_;
	float invincibleTime_;

	ParticleEmitter reviveEmit1_;
	ParticleEmitter reviveEmit2_;

	SoundData revivalSE_;

public:
	//コンストラクタ
	PlayerReSpown(Player* boss);
	~PlayerReSpown();

	void Update()override;
	void Debug()override;

};
