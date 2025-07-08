#pragma once
#include "BaseBossBehavior.h"


class BossJumpAttack : public BaseBossBehavior {
public:
	BossJumpAttack(Boss* pBoss,int count = 2);
	~BossJumpAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	int jumpCount_ = 0;
	int nowJumpCount_ = 0;
};
