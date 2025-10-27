#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの基本攻撃の行動クラス
/// </summary>
class BossAttack : public BaseBossBehavior {
public:
	BossAttack(Boss* pBoss);
	~BossAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	bool isAttack_ = true;
	float coolTime_ = 30.0f;
	SoundData* attackSE_;

};
