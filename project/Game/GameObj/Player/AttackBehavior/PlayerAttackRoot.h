#pragma once
#include "BasePlayerAttackBehavior.h"


class PlayerAttackRoot : public BasePlayerAttackBehavior {
public:
	PlayerAttackRoot(Player* pPlayer);
	~PlayerAttackRoot();

	enum class Step {
		ROOT,
		CHAREGE,
		STRONGSHOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float chargeTime_;

};
