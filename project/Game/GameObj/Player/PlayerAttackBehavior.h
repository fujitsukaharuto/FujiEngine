#pragma once
#include "Game/OriginState.h"
#include "Math/Vector3.h"
#include <vector>
#include "Audio/Audio.h"

class Player;


class PlayerAttackBehavior : public OriginState {
public:
	PlayerAttackBehavior(Player* pPlayer);
	~PlayerAttackBehavior()override;

	void Initialize()override;
	void Update()override;


	void Attack();
	void Direction();

	void EndInit();

private:

	Player* pPlayer_;
	bool isAttack_ = true;

	float attackSpeed_ = 0.1f;

	int comboIndex_ = 0;
	bool isChain_ = false;

	//float attackT_ = 30.0f;
	float attackT1_ = 40.0f;
	float attackT2_ = 0.0f;
	float attackT3_ = 0.0f;
	float attackT4_ = 0.0f;
	float attackT5_ = 0.0f;

	float attackLimmit1_ = 12.0f;
	float attackLimmit2_ = 12.0f;
	float attackLimmit3_ = 12.0f;
	float attackLimmit4_ = 12.0f;
	float attackLimmit5_ = 15.0f;

	bool isAttack1_ = true;
	bool isAttack2_ = false;
	bool isAttack3_ = false;
	bool isAttack4_ = false;
	bool isAttack5_ = false;

	std::vector<Vector3> attackPoint1_;
	std::vector<Vector3> attackPoint2_;
	std::vector<Vector3> attackPoint3_;
	std::vector<Vector3> attackPoint4_;
	std::vector<Vector3> attackPoint5_;

};
