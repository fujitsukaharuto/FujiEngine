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
	float stopReng_ = 20.0f;
	float dushReng_ = 0.0f;
	float coolTime_ = 30.0f;

	float chargeTime_ = 0.0f;
	float maxCharegeTime_ = 90.0f;
};
