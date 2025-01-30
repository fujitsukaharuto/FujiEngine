#pragma once

#include"BaseLifeUIBehavior.h"
#include"math/Easing.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class LifeUIRoot : public BaseLifeUIBehavior {
private:
	enum class Step {
		WAIT,
		ATTACKCHAREGE,
		STRONGATTACK,
	};

/// ===================================================
///  private varians
/// ===================================================
	Step step_;


public:
	//コンストラクタ
	LifeUIRoot(LifeUI* boss);
	~LifeUIRoot();

	void Update()override;
	void Debug()override;

};
