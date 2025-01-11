#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"

class PlayerAttackRoot : public BasePlayerAttackBehavior {
private:

/// ===================================================
///  private varians
/// ===================================================


public:
	//コンストラクタ
	PlayerAttackRoot(Player* boss);
	~PlayerAttackRoot();

	void Update()override;
	void Debug()override;

	void KikAttackState();
};
