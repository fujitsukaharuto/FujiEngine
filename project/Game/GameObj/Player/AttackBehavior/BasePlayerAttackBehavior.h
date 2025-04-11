#pragma once
#include<string>

class Player;
class BasePlayerAttackBehavior {
public:
	BasePlayerAttackBehavior(Player* player) :pPlayer_(player) {}
	virtual ~BasePlayerAttackBehavior() {}
	virtual void Update() = 0;
	virtual void Debug() = 0;

protected:

	Player* pPlayer_ = nullptr;

};