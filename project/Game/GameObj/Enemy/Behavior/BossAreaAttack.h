#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの範囲攻撃の行動クラス
/// </summary>
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

	Audio::SoundData* areaAttackSE_;

};
