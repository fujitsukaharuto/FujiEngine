#pragma once
#include "Game/OriginState.h"

class Player;


class PlayerDefaultBehavior : public OriginState {
public:
	PlayerDefaultBehavior(Player* pPlayer);
	~PlayerDefaultBehavior()override;

	void Initialize()override;
	void Update()override;


	void Move();

private:

	Player* pPlayer_;

};
