#pragma once
#include "BasePlayerAttackBehavior.h"
#include "Engine/Particle/ParticleEmitter.h"

class PlayerAttackRoot : public BasePlayerAttackBehavior {
public:
	PlayerAttackRoot(Player* pPlayer);
	~PlayerAttackRoot();

	enum class Step {
		ROOT,
		CHAREGE,
		STRONGSHOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float chargeTime_;

	// Emitter
	ParticleEmitter* charge1_;
	ParticleEmitter* charge2_;
	ParticleEmitter* charge3_;
	ParticleEmitter* chargeLight_;

};
