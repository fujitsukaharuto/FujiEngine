#pragma once

#include<string>

class UFO;
class BaseUFOBehavior {
protected:
	//振る舞い名
	std::string name_;
	//ボス
	UFO* pUFO_ = nullptr;
public:

	BaseUFOBehavior(const std::string& name, UFO* player) :name_(name), pUFO_(player) {}
	
	virtual ~BaseUFOBehavior() {}

	virtual void Update()=0;
	virtual void Debug() = 0;
};