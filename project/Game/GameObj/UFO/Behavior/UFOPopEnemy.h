#pragma once

#include"BaseUFOrBehavior.h"
#include"math/Easing.h"

class UFOPopEnemy : public BaseUFOBehavior {
private:
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
