#pragma once

#include"BaseUFOrBehavior.h"
#include"math/Easing.h"

class UFORoot : public BaseUFOBehavior {
private:
	float speed_;
	/// ===================================================
	///private varians
	/// ===================================================

public:
	//コンストラクタ
	UFORoot(UFO* boss);
	~UFORoot();

	void Update()override;

	void Debug()override;

};
