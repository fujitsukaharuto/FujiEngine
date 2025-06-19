#pragma once
#include "BaseBossBehavior.h"


class BossSwordAttack : public BaseBossBehavior {
public:
	BossSwordAttack(Boss* pBoss);
	~BossSwordAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float coolTime_;
	int attackCount_;

};
