#pragma once

#include"BaseUFOState.h"
#include"math/Easing.h"

class UFOPopWait : public BaseUFOState {
private:
	enum class Step {
		POP,
		ANIMATION,
		RETURNROOT,
	};
private:
	Step step_;
	float speed_;
	Easing easing_;

	float easeDirection_;
	float scalingEaseMax_;
	float scalingEaseTime_;

	float rTIme_;
	float rotateAmplitude_;
	float rotateFrequency_;

	/// ===================================================
	///private varians
	/// ===================================================

public:
	//コンストラクタ
	UFOPopWait(UFO* boss);
	~UFOPopWait();

	void Update()override;

	void Debug()override;

	void ExPationing();

	void Rotation();

};
