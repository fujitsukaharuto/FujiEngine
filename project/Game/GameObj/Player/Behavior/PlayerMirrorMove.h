#pragma once
#include "BasePlayerBehavior.h"


/// <summary>
/// Playerのミラー移動の行動クラス
/// </summary>
class PlayerMirrorMove : public BasePlayerBehavior {
public:
	PlayerMirrorMove(Player* pPlayer);
	~PlayerMirrorMove();

	enum class Step {
		MIRRORMOVE,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;

};
