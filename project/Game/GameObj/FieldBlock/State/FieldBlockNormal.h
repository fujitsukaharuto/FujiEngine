#pragma once

#include"BaseFieldBlockState.h"
#include"Easing.h"
#include"Particle/ParticleEmitter.h"

class FieldBlockNormal: public BaseFieldBlockState {
private:
	enum class Step {
		NORMAL,
		WAIT,
		DAUNGEROUSMODE,
	};
private:

	Step step_;
	
	ParticleEmitter ChangeEmit_;
	float waitTime_;
	float kwaitTime_;

public:
	//コンストラクタ
	FieldBlockNormal(FieldBlock* boss);
	~FieldBlockNormal();


	void Update()override;
	void Debug()override;


};
