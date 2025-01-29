#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"

class PlayerRoot : public BasePlayerBehavior {
private:

	enum class Step {
		ROOT,
		GOJUMP,
	};

/// ===================================================
///private varians
/// ===================================================
	Step step_;
	

public:
	//コンストラクタ
	PlayerRoot(Player* boss);
	~PlayerRoot();

	void Update()override;
	void JumpForJoyState();
	void Debug()override;

};
