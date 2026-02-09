#pragma once
#include "BasePlayerBehavior.h"

/// <summary>
/// Playerジャンプ状態
/// </summary>
class PlayerJump : public BasePlayerBehavior {
public:
	PlayerJump(Player* pPlayer);
	~PlayerJump();

	enum class Step {
		JUMP,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float speed_;
	bool isSecondJump_ = true;

};
