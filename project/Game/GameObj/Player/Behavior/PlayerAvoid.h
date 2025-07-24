#pragma once
#include "BasePlayerBehavior.h"


class PlayerAvoid : public BasePlayerBehavior {
public:
	PlayerAvoid(Player* pPlayer);
	~PlayerAvoid();

	enum class Step {
		AVOID,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float avoidTime_;

};
