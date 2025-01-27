#pragma once

#include"BaseUFOrBehavior.h"
#include"math/Easing.h"

class UFODamage : public BaseUFOBehavior {
private:
	enum class Step {
		HITBACK,
		RETURNROOT,
	};
	/// ===================================================
	///private varians
	/// ===================================================
	Step step_;
	Easing easing_;
	Vector3 initPos_;
	float stopTime_;

public:
	//コンストラクタ
	UFODamage(UFO* boss);
	~UFODamage();

	void Update()override;

	void Debug()override;

};
