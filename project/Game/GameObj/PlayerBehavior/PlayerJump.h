#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"
#include "Audio/Audio.h"

class PlayerJump : public BasePlayerBehavior {
private:
	float speed_;
	/// ===================================================
	///private varians
	/// ===================================================

	SoundData jumpSE_;

public:
	//コンストラクタ
	PlayerJump(Player* boss);
	~PlayerJump();

	void Update()override;

	void Debug()override;

};
