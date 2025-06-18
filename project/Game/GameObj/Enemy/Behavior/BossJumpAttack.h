#pragma once
#include "BaseBossBehavior.h"


class BossJumpAttack : public BaseBossBehavior {
public:
	BossJumpAttack(Boss* pBoss);
	~BossJumpAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;

};
