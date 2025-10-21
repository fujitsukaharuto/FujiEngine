#pragma once
#include "BasePlayerBehavior.h"

/// <summary>
/// Player基本状態
/// </summary>
class PlayerRoot : public BasePlayerBehavior {
public:
	PlayerRoot(Player* pPlayer);
	~PlayerRoot();

	enum class Step {
		ROOT,
		TOJUMP,
		TOAVOID,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float speed_;

};
