#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"

class PlayerRoot : public BasePlayerBehavior {
private:

/// ===================================================
///private varians
/// ===================================================

	

public:
	//コンストラクタ
	PlayerRoot(Player* boss);
	~PlayerRoot();

	void Update()override;

	void Debug()override;

};
