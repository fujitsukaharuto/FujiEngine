#pragma once

#include"BaseFieldBlockState.h"
#include"Easing.h"
#include "Particle/ParticleEmitter.h"

class FieldBlockDaungerous: public BaseFieldBlockState {
private:
	enum class Step {
		DAUNGEROUSMODE,
		GONORMAL,
	};
private:

	Step step_;
	float restoreTime_;

	ParticleEmitter darkEmit_;
	
public:
	//コンストラクタ
	FieldBlockDaungerous(FieldBlock* boss);
	~FieldBlockDaungerous();


	void Update()override;
	void Debug()override;


};
