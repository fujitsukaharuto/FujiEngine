#pragma once

#include<string>

class UFO;
class BaseUFOState{
protected:
	//振る舞い名
	std::string name_;
	//ボス
	UFO* pUFO_ = nullptr;
public:

	BaseUFOState(const std::string& name, UFO* player) :name_(name), pUFO_(player) {}
	
	virtual ~BaseUFOState() {}

	virtual void Update()=0;
	virtual void Debug() = 0;
};