#pragma once
#include "BasePlayerBehavior.h"


/// <summary>
/// Playerの掴み行動の行動クラス
/// </summary>
class PlayerGrabBehavior : public BasePlayerBehavior {
public:
	PlayerGrabBehavior(Player* pPlayer);
	~PlayerGrabBehavior();

	/// <summary>
	/// 掴み行動の進行状態、掴んでから移動させる
	/// </summary>
	enum class Step {
		GRAB,
		GRABMOVE,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float speed_;

};
