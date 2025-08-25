#pragma once
#include<string>

enum class AttackPattern {
	Beam,
	Wave,
	JumpAttack,
	SwordAttack,
	AreaAttack,
	ArrowAttack,
	FallRod,
	// 今後追加
};

struct AttackInfo {
	AttackPattern pattern;
	float weight;
};

class Boss;
class BaseBossBehavior {
public:
	BaseBossBehavior(Boss* boss) : pBoss_(boss) {}
	virtual ~BaseBossBehavior() {}
	virtual void Update() = 0;
	virtual void Debug() = 0;

protected:

	Boss* pBoss_ = nullptr;

};