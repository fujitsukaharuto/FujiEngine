#pragma once

#include"BaseUFOState.h"
#include"math/Easing.h"
#include "Audio/Audio.h"

class UFOMissilePop : public BaseUFOState {
private:
	enum class Step {
		LIGHTUP,
		WAIT,
		LIGHTCLOSE,
		RETURNROOT,
	};
private:
	Step step_;

	float waitTime_;
	float kWaitTime_;
	
	Vector3 initScale_;
	Vector3 scaleUnderPop_;
	Easing lightUpEasing_;
	Easing lightCloseEasing_;

	SoundData firstSE_;

	/// ===================================================
	///private varians
	/// ===================================================

public:
	//コンストラクタ
	UFOMissilePop(UFO* boss);
	~UFOMissilePop();

	void Update()override;

	void Debug()override;

};
