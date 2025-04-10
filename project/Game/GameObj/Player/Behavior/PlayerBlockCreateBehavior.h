#pragma once
#include "BasePlayerBehavior.h"


class PlayerBlockCreateBehavior : public BasePlayerBehavior {
public:
	PlayerBlockCreateBehavior(Player* pPlayer);
	~PlayerBlockCreateBehavior();

	enum class Step {
		BLOCKCREATE,
		BLOCKPOP,
		BLOCKPOPING,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	int dir_;

};
