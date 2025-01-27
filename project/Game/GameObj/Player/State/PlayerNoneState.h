#pragma once

#include"BasePlayerState.h"
#include"math/Easing.h"

class PlayerNoneState : public BasePlayerState {
private:

	/// ===================================================
	///private varians
	/// ===================================================



public:
	//コンストラクタ
	PlayerNoneState(Player* boss);
	~PlayerNoneState();

	void Update()override;
	void Debug()override;

};
