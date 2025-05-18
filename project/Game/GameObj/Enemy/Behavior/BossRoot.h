#pragma once
#include "BaseBossBehavior.h"


class BossRoot : public BaseBossBehavior {
public:
	BossRoot(Boss* pBoss);
	~BossRoot();

	enum class Step {
		ROOT,
		TOATTACK,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float cooldown_;
};
