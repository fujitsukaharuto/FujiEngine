#pragma once
#include<string>

class Player;

/// <summary>
/// PlayerAttackBehavior基底クラス
/// </summary>
class BasePlayerAttackBehavior {
public:
	BasePlayerAttackBehavior(Player* player) :pPlayer_(player) {}
	virtual ~BasePlayerAttackBehavior() {}
	virtual void Update() = 0;
	virtual void Debug() = 0;

	virtual void ResetParam() = 0;
	virtual void StopSE() = 0;

protected:

	Player* pPlayer_ = nullptr;

};