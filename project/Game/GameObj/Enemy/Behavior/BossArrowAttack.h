#pragma once
#include "BaseBossBehavior.h"


class BossArrowAttack : public BaseBossBehavior {
public:
	BossArrowAttack(Boss* pBoss, bool beforArrow = false);
	~BossArrowAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	bool isAttack_ = true;
	float coolTime_ = 120.0f;

	bool isbeforArrow_ = false;
	float beforWait_ = 0.0f;

};
