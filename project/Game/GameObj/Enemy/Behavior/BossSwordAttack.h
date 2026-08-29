#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの連続攻撃の行動クラス
/// </summary>
class BossSwordAttack : public BaseBossBehavior {
public:
	BossSwordAttack(Boss* pBoss);
	~BossSwordAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;

private:

	Step step_;
	float coolTime_;
	int attackCount_;
	Audio::SoundData* attackSE_;

};
