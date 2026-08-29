#pragma once
#include "BaseBossBehavior.h"

/// <summary>
/// Bossの基本状態クラス
/// </summary>
class BossRoot : public BaseBossBehavior {
public:
	BossRoot(Boss* pBoss);
	~BossRoot();

	/// <summary>
	/// 通常状態の進行状態、待機してから攻撃へ移る
	/// </summary>
	enum class Step {
		ROOT,
		TOATTACK,
	};

	void Update()override;

	/// <summary>
	/// 次の攻撃の選択
	/// </summary>
	AttackPattern ChooseNextAttack();

	/// <summary>
	/// stringからAttackPatternへの変更
	/// </summary>
	AttackPattern ToAttackPattern(const std::string& name);

private:

	Step step_;
	float cooldown_;
};
