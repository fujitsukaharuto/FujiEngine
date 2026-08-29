#pragma once
#include "BasePlayerBehavior.h"

/// <summary>
/// Player回避状態
/// </summary>
class PlayerAvoid : public BasePlayerBehavior {
public:
	PlayerAvoid(Player* pPlayer);
	~PlayerAvoid();

	enum class Step {
		AVOID,
		TOROOT,
	};

	void Update()override;

private:

	Step step_;
	float avoidTime_;

};
