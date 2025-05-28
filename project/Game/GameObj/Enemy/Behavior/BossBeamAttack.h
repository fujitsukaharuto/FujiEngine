#pragma once
#include "BaseBossBehavior.h"
#include "Engine/Particle/ParticleEmitter.h"


class BossBeamAttack : public BaseBossBehavior {
public:
	BossBeamAttack(Boss* pBoss);
	~BossBeamAttack();

	enum class Step {
		CHARGE,
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;

};
