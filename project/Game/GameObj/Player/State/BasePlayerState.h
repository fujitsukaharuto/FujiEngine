#pragma once

#include<string>

class Player;
class BasePlayerState {
protected:
	//振る舞い名
	std::string name_;
	//ボス
	Player* pPlayer_ = nullptr;
public:

	BasePlayerState(const std::string& name, Player* player) :name_(name), pPlayer_(player) {}
	
	virtual ~BasePlayerState() {}

	virtual void Update()=0;

	virtual void Debug() = 0;
};