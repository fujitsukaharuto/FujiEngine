#pragma once

#include"BasePlayerState.h"
#include"math/Easing.h"

class PlayerDeath : public BasePlayerState {
private:
	enum class Step {
		DEATH,
		WAIT,
		GORESPOWN,
	};
/// ===================================================
///private varians
/// ===================================================
	Step step_;
	float waitTime_;
	

public:
	//コンストラクタ
	PlayerDeath(Player* boss);
	~PlayerDeath();

	void Update()override;
	void Debug()override;

};
