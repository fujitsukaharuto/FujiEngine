#pragma once

#include"BaseLifeUIBehavior.h"
#include"math/Easing.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class LifeUIDeath : public BaseLifeUIBehavior {
private:
	enum class Step {
		WAIT,
		UIMOVE,
		BREAK,
		RETURN,
		GOROOT,
	};

	/// ===================================================
	///  private varians
	/// ===================================================
	Step  step_;
	float waitTime_;
	float deathMoveTime_;
	float returnMoveTime_;
public:
	//コンストラクタ
	LifeUIDeath(LifeUI* boss);
	~LifeUIDeath();

	void Update()override;
	void Debug()override;

};
