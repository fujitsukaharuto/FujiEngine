#pragma once

#include<string>

class Player;
class BasePlayerBehavior {
protected:
	//振る舞い名
	std::string name_;
	//ボス
	Player* pPlayer_ = nullptr;
public:

	BasePlayerBehavior(const std::string& name, Player* player) :name_(name), pPlayer_(player) {}
	
	virtual ~BasePlayerBehavior() {}

	virtual void Update()=0;

	virtual void Debug() = 0;
};