#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの落雷攻撃の行動クラス
/// </summary>
class BossRodFall : public BaseBossBehavior {
public:
	BossRodFall(Boss* pBoss);
	~BossRodFall();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;

private:

	Step step_;
	bool isAttack_ = true;
	float coolTime_ = 160.0f;

	bool isChange_ = false;

};
