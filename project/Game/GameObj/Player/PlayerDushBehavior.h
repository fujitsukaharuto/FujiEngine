#pragma once
#include "Game/OriginState.h"
#include "Math/Vector3.h"
#include <vector>

class Player;


class PlayerDushBehavior : public OriginState {
public:
	PlayerDushBehavior(Player* pPlayer);
	~PlayerDushBehavior()override;

	void Initialize()override;
	void Update()override;


	void Dush();
	void Direction();

	void EndInit();

	float CustomEasing(float t);

private:

	Player* pPlayer_;
	bool isAttack_ = true;

	int comboIndex_ = 0;
	bool isChain_ = false;

	//float attackT_ = 30.0f;
	float attackT1_ = 30.0f;
	float attackT2_ = 0.0f;


	float attackLimmit1_ = 12.0f;
	float attackLimmit2_ = 12.0f;


	bool isAttack1_ = true;
	bool isAttack2_ = false;


	std::vector<Vector3> attackPoint1_;
	std::vector<Vector3> attackPoint2_;

	

};
