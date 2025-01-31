#pragma once

#include"Sprite.h"
#include"Easing.h"

class GameOver {
public:
	enum class Step {
		FADEIN,
		UISELECT,
		GOTITLE,
		FADEOUT,
	};
	enum class SelectMode {
		GOTITLE,
		GOGAME,
	};

private:
	Step step_;
	SelectMode selectMode_;
	std::unique_ptr<Sprite> overPaneru_;
	std::unique_ptr<Sprite> selectArrow_;
	Vector3 selectPos_;

	/// fade
	Easing fadeInEaseT_;
	Easing fadeOutEaseT_;
	float alpha_;

	/// selectPos
	const Vector3 gameSelectPos_ = { 251.6f,444.0f ,0};
	const Vector3 titleSelectPos_ = { 251.6f,584.5f ,0 };
	float selectPosOffSet_;
	Easing offsetEasing_;
	float  easeDirection_;

	/// frag
	bool isTitleBack_;
	bool isGameBack_;
public:
	void Init();
	void Update();
	void Draw();
	void Debug();
	void OffsetMove();
	void SetStepStart();
	///==============================================================
	/// getter method
	///================================================================
	bool GetIsTitleBack()const { return isTitleBack_; }
	bool GetIsGameBack()const { return isGameBack_; }
};