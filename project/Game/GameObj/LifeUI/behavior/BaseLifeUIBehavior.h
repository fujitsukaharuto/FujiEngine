#pragma once

#include<string>

class LifeUI;
class BaseLifeUIBehavior {
protected:
	//振る舞い名
	std::string name_;
	//ボス
	LifeUI* pLifeUI_ = nullptr;
public:

	BaseLifeUIBehavior(const std::string& name, LifeUI* player) :name_(name), pLifeUI_(player) {}
	
	virtual ~BaseLifeUIBehavior() {}

	virtual void Update()=0;

	virtual void Debug() = 0;
};