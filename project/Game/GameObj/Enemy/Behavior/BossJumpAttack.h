#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossのジャンプ攻撃の行動クラス
/// </summary>
class BossJumpAttack : public BaseBossBehavior {
public:
	BossJumpAttack(Boss* pBoss,int count = 2);
	~BossJumpAttack();

	enum class Step {
		ATTACK,
		TOROOT,
	};

	void Update()override;

private:

	Step step_;
	int jumpCount_ = 0;
	int nowJumpCount_ = 0;
};
