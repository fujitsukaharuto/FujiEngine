#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"

class PlayerAttackRoot : public BasePlayerAttackBehavior {
private:
	enum class Step {
		WAIT,
		ATTACK,
	};

/// ===================================================
///  private varians
/// ===================================================
	Step step_;
	float waitTime_;

public:
	//コンストラクタ
	PlayerAttackRoot(Player* boss);
	~PlayerAttackRoot();

	void Update()override;
	void Debug()override;

	void KikAttackState();
};
