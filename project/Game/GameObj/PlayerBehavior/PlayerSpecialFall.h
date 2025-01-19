#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"

class PlayerSpecialFall : public BasePlayerAttackBehavior {
private:
	enum class Step {
		ANTIPATION,
		FALL,
		POSTACTION,
		RETURNROOT,
	};

/// ===================================================
///  private varians
/// ===================================================
	Step step_;
	float antiTime_;
	float fallTime_;

	float rotationZ_;
	float rotateSpeed_;
	float postJumpSpeed_;
public:
	//コンストラクタ
	PlayerSpecialFall(Player* boss);
	~PlayerSpecialFall();

	void Update()override;
	void Debug()override;

	void KikAttackState();
};
