#pragma once
#include "BasePlayerBehavior.h"


class PlayerGrabBehavior : public BasePlayerBehavior {
public:
	PlayerGrabBehavior(Player* pPlayer);
	~PlayerGrabBehavior();

	enum class Step {
		GRAB,
		GRABMOVE,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float speed_;

};
