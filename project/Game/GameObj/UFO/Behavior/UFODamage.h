#pragma once

#include"BaseUFOrBehavior.h"
#include"math/Easing.h"
#include "Audio/Audio.h"

class UFODamage : public BaseUFOBehavior {
private:
	enum class Step {
		HITBACK,
		RETURN,
		RETURNROOT,
	};
	/// ===================================================
	///private varians
	/// ===================================================
	Step step_;
	Easing easing_;
	Easing returnEase_;
	Vector3 initPos_;
	float stopTime_;

	SoundData damageSE_;

public:
	//コンストラクタ
	UFODamage(UFO* boss);
	~UFODamage();

	void Update()override;

	void Debug()override;

};
