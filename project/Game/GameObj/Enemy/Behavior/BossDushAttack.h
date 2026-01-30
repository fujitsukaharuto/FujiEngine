#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの基本攻撃の行動クラス
/// </summary>
class BossDushAttack : public BaseBossBehavior {
public:
	BossDushAttack(Boss* pBoss, bool is = false);
	~BossDushAttack();

	enum class Step {
		CHARGE,
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	bool isAttack_ = true;
	bool isPreDush_ = false;
	bool isNear_ = false;
	float stopRange_ = 70.0f;
	float dushRange_ = 0.0f;
	float coolTime_ = 30.0f;

	float chargeTime_ = 0.0f;
	float maxChargeTime_ = 90.0f;
	float startWaitTime_ = 0.0f;
	float maxStartWaitTime_ = 10.0f;

};
