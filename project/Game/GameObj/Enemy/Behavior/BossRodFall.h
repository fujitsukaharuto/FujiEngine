#pragma once
#include "BaseBossBehavior.h"


class BossRodFall : public BaseBossBehavior {
public:
	BossRodFall(Boss* pBoss);
	~BossRodFall();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	bool isAttack_ = true;
	float coolTime_ = 160.0f;

	bool isChange_ = false;

};
