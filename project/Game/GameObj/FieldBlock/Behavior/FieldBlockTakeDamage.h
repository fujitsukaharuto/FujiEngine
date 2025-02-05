#pragma once

#include"BaseFieldBlockBehavior.h"
#include"Easing.h"
#include "Particle/ParticleEmitter.h"

class FieldBlockTakeDamage : public BaseFieldBlockBehavior {
private:
	enum class Step {
		DAMAGE,
		NODAMAGE,
	};
private:

	Step step_;
	float collTime_;
	Vector3 originPos_{};

	ParticleEmitter emit_;

public:
	//コンストラクタ
	FieldBlockTakeDamage(FieldBlock* boss);
	~FieldBlockTakeDamage();


	void Update()override;
	void Debug()override;


};
