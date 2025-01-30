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

	Easing fadeInEaseT_;
	Easing fadeOutEaseT_;
	float alpha_;


	/// frag
	bool isTitleBack_;
	bool isGameBack_;
public:
	void Init();
	void Update();
	void Draw();
	void Debug();
	///==============================================================
	/// getter method
	///================================================================
	bool GetIsTitleBack()const { return isTitleBack_; }
	bool GetIsGameBack()const { return isGameBack_; }
};