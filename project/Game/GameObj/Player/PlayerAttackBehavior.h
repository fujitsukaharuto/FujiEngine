#pragma once
#include "Game/OriginState.h"
#include "Math/Vector3.h"
#include <vector>

class Player;


class PlayerAttackBehavior : public OriginState {
public:
	PlayerAttackBehavior(Player* pPlayer);
	~PlayerAttackBehavior()override;

	void Initialize()override;
	void Update()override;


	void Attack();

private:

	Player* pPlayer_;
	bool isAttack_ = true;

	int comboIndex_ = 0;
	bool isChain_ = false;

	//float attackT_ = 30.0f;
	float attackT1_ = 40.0f;
	float attackT2_ = 0.0f;
	float attackT3_ = 0.0f;
	float attackT4_ = 0.0f;

	bool isAttack1_ = true;
	bool isAttack2_ = false;
	bool isAttack3_ = false;
	bool isAttack4_ = false;

	std::vector<Vector3> attackPoint1_;
	std::vector<Vector3> attackPoint2_;
	std::vector<Vector3> attackPoint3_;
	std::vector<Vector3> attackPoint4_;

};
