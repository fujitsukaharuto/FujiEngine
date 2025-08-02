#pragma once
#include "BaseBossBehavior.h"


class BossAreaAttack : public BaseBossBehavior {
public:
	BossAreaAttack(Boss* pBoss);
	~BossAreaAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	bool isAttack_ = true;
	float coolTime_ = 80.0f;

};
