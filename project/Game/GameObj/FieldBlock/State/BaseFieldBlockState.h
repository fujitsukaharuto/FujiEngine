#pragma once

#include<string>

class FieldBlock;
class BaseFieldBlockState{
protected:
	//振る舞い名
	std::string name_;
	//ボス
	FieldBlock* pFieldBlock_ = nullptr;
public:

	BaseFieldBlockState(const std::string& name, FieldBlock* boss) :name_(name), pFieldBlock_(boss) {}
	
	virtual ~BaseFieldBlockState() {}

	virtual void Update()=0;

	virtual void Debug() = 0;

};