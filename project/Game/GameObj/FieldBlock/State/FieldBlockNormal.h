#pragma once

#include"BaseFieldBlockState.h"
#include"Easing.h"

class FieldBlockNormal: public BaseFieldBlockState {
private:
	enum class Step {
		NORMAL,
		DAUNGEROUSMODE,
	};
private:

	Step step_;


public:
	//コンストラクタ
	FieldBlockNormal(FieldBlock* boss);
	~FieldBlockNormal();


	void Update()override;
	void Debug()override;


};
