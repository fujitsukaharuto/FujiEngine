#pragma once
#include "BasePlayerBehavior.h"


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
