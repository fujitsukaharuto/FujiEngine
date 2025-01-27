#pragma once

#include"BasePlayerState.h"
#include"math/Easing.h"

class PlayerReSpown : public BasePlayerState {
private:

	enum class Step {
	   RESPAWN,
	   GOROOT,
	};

	/// ===================================================
	///private varians
	/// ===================================================
	Step  step_;
	Easing ease_;
	float invincibleTime_;


public:
	//コンストラクタ
	PlayerReSpown(Player* boss);
	~PlayerReSpown();

	void Update()override;
	void Debug()override;

};
