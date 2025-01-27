#pragma once

#include<string>

class BaseEnemy;
class BaseEnemyState{
protected:
	//振る舞い名
	std::string name_;
	//ボス
	BaseEnemy* pBaseEnemy_ = nullptr;
public:

	BaseEnemyState(const std::string& name, BaseEnemy* boss) :name_(name), pBaseEnemy_(boss) {}
	
	virtual ~BaseEnemyState() {}

	virtual void Update()=0;

	virtual void Debug() = 0;

};