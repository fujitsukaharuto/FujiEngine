#pragma once

#include"BaseUFOState.h"
#include"math/Easing.h"

class UFOMissilePop : public BaseUFOState {
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
