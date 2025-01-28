#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"


class PlayerKikAttack : public BasePlayerAttackBehavior {
private:
	enum class Step {
		KIK,
		RETUNROOT,
	};
/// ===================================================
///  private varians
/// ===================================================
	Step step_;
	float kikTime_;
	float kikRotateZ_;

public:
	//コンストラクタ
	PlayerKikAttack(Player* boss);
	~PlayerKikAttack();

	void Update()override;
	void Debug()override;
	void KikRotation();

	
};
