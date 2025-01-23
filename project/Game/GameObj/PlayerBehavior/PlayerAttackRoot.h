#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"
#include "Particle/ParticleEmitter.h"

class PlayerAttackRoot : public BasePlayerAttackBehavior {
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
	float waitTime_;

	float chargeTime_;

	ParticleEmitter charge1_;
	ParticleEmitter charge2_;

	ParticleEmitter kiran1_;
	ParticleEmitter kiran2_;


public:
	//コンストラクタ
	PlayerAttackRoot(Player* boss);
	~PlayerAttackRoot();

	void Update()override;
	void Debug()override;

	void ChargeForJoyStick();
	void AtttackForJoyStick();
};
