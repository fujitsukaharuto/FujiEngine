#pragma once
#include<string>

class Player;
class BasePlayerBehavior {
public:
	BasePlayerBehavior(Player* player) :pPlayer_(player) {}
	virtual ~BasePlayerBehavior() {}
	virtual void Update() = 0;
	virtual void Debug() = 0;

protected:

	Player* pPlayer_ = nullptr;

};