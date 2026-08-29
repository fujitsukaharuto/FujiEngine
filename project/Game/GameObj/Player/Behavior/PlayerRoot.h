#pragma once
#include "BasePlayerBehavior.h"

/// <summary>
/// Player基本状態
/// </summary>
class PlayerRoot : public BasePlayerBehavior {
public:
	PlayerRoot(Player* pPlayer);
	~PlayerRoot();

	/// <summary>
	/// 通常状態の進行状態、待機からジャンプや回避へ移る
	/// </summary>
	enum class Step {
		ROOT,
		TOJUMP,
		TOAVOID,
	};

	void Update()override;

private:

	Step step_;
	float speed_;

};
