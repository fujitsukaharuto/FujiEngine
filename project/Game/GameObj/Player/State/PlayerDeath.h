#pragma once

#include"BasePlayerState.h"
#include"math/Easing.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class PlayerDeath : public BasePlayerState {
private:
	enum class Step {
		DEATH,
		WAIT,
		GORESPOWN,
	};
/// ===================================================
///private varians
/// ===================================================
	Step step_;
	float waitTime_;
	
	ParticleEmitter deadEmit1_;
	ParticleEmitter deadEmit2_;

	SoundData deadSE_;

public:
	//コンストラクタ
	PlayerDeath(Player* boss);
	~PlayerDeath();

	void Update()override;
	void Debug()override;

};
