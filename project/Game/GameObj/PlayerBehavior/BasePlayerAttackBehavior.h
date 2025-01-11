#pragma once

#include<string>

class Player;
class BasePlayerAttackBehavior {
protected:
	//振る舞い名
	std::string name_;
	//ボス
	Player* pPlayer_ = nullptr;
public:

	BasePlayerAttackBehavior(const std::string& name, Player* player) :name_(name), pPlayer_(player) {}
	
	virtual ~BasePlayerAttackBehavior() {}

	virtual void Update()=0;

	virtual void Debug() = 0;
};