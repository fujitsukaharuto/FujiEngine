#pragma once

#include"BaseUFOrBehavior.h"
#include"math/Easing.h"

class UFOPopEnemy : public BaseUFOBehavior {
private:
	enum class Step {
		POP,
		RETURNROOT,
	};
private:
	Step step_;
	float speed_;
	/// ===================================================
	///private varians
	/// ===================================================

public:
	//コンストラクタ
	UFOPopEnemy(UFO* boss);
	~UFOPopEnemy();

	void Update()override;

	void Debug()override;

};
