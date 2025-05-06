#pragma once
#include<string>

class Boss;
class BaseBossBehavior {
public:
	BaseBossBehavior(Boss* boss) : pBoss_(boss) {}
	virtual ~BaseBossBehavior() {}
	virtual void Update() = 0;
	virtual void Debug() = 0;

protected:

	Boss* pBoss_ = nullptr;

};