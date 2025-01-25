#pragma once

#include"BaseFieldBlockBehavior.h"
#include"Easing.h"

class FieldBlockNoDamage : public BaseFieldBlockBehavior {
private:
	enum class Step {
		DAMAGE,
		RETUNROOT,
	};
private:

	Step step_;
	


public:
	//コンストラクタ
	FieldBlockNoDamage(FieldBlock* boss);
	~FieldBlockNoDamage();


	void Update()override;
	void Debug()override;


};
