#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"


class PlayerKikAttack : public BasePlayerBehavior {
private:

/// ===================================================
///  private varians
/// ===================================================
	
	

public:
	//コンストラクタ
	PlayerKikAttack(Player* boss);
	~PlayerKikAttack();

	void Update()override;
	void Debug()override;

};
