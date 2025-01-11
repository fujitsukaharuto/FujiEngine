#pragma once

#include"BaseUFOrBehavior.h"
#include"math/Easing.h"

class UFORoot : public BaseUFOBehavior {
private:
	enum class Step {
		WAIT,
		CHANGEPOP,
	};
	/// ===================================================
	///private varians
	/// ===================================================
	Step step_;
	float waitTime_;

public:
	//コンストラクタ
	UFORoot(UFO* boss);
	~UFORoot();

	void Update()override;

	void Debug()override;

};
