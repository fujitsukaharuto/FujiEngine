#pragma once

#include<string>

class BaseEnemy;
class BaseEnemyBehaivor{
protected:
	//振る舞い名
	std::string name_;
	//ボス
	BaseEnemy* pBaseEnemy_ = nullptr;
public:

	BaseEnemyBehaivor(const std::string& name, BaseEnemy* boss) :name_(name), pBaseEnemy_(boss) {}
	
	virtual ~BaseEnemyBehaivor() {}

	virtual void Update()=0;

	virtual void Debug() = 0;

};