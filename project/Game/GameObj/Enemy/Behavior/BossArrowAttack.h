#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの遠距離攻撃の行動クラス
/// </summary>
class BossArrowAttack : public BaseBossBehavior {
public:
	BossArrowAttack(Boss* pBoss, bool beforArrow = false);
	~BossArrowAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;

private:

	Step step_;
	bool isAttack_ = true;
	float coolTime_ = 100.0f;

	bool isbeforArrow_ = false;
	float beforWait_ = 0.0f;

};
