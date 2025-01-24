#pragma once

#include"BaseFieldBlockState.h"
#include"Easing.h"

class FieldBlockRoot: public BaseFieldBlockState {
private:
	enum class Step {
		BLOW,
		RETUNROOT,
	};
private:

	Step step_;
	


public:
	//コンストラクタ
	FieldBlockRoot(FieldBlock* boss);
	~FieldBlockRoot();


	void Update()override;
	void Debug()override;


};
