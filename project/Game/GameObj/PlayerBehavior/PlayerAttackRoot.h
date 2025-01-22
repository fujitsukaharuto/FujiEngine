#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"

class PlayerAttackRoot : public BasePlayerAttackBehavior {
private:
	enum class Step {
		WAIT,
		ATTACKCHAREGE,
		STRONGATTACK,
	};

/// ===================================================
///  private varians
/// ===================================================
	Step step_;
	float waitTime_;

	float chargeTime_;
public:
	//コンストラクタ
	PlayerAttackRoot(Player* boss);
	~PlayerAttackRoot();

	void Update()override;
	void Debug()override;

	void ChargeForJoyStick();
	void AtttackForJoyStick();
};
