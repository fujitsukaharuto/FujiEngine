#pragma once
#include "BaseBossBehavior.h"

enum class AttackPattern {
	Beam,
	Wave,
	JumpAttack,
	SwordAttack,
	// 今後追加
};

struct AttackInfo {
	AttackPattern pattern;
	float weight;
};

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
	AttackPattern ChooseNextAttack();

private:

	Step step_;
	float cooldown_;
};
