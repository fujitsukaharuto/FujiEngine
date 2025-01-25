#pragma once

#include"BaseFieldBlockBehavior.h"
#include"Easing.h"

class FieldBlockTakeDamage : public BaseFieldBlockBehavior {
private:
	enum class Step {
		DAMAGE,
		NODAMAGE,
	};
private:

	Step step_;
	float collTime_;


public:
	//コンストラクタ
	FieldBlockTakeDamage(FieldBlock* boss);
	~FieldBlockTakeDamage();


	void Update()override;
	void Debug()override;


};
