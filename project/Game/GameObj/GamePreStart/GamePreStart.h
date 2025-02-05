#pragma once

#include"Sprite.h"
#include"Easing.h"

class UFO;
class SkyDome;
class BackGround;
class GamePreStart {
public:
	enum class Step {
		WAIT,
		GOUPGROUND,
		APEARUFO,
		AIMWAIT,
		AIMOPEN,
		AIMKIKOPEN,
		CLOSEWAIT,
		AIMCLOSE,
		END,
	};

private:
	UFO* pUFO_;
	SkyDome* pSkyDome_;
	BackGround* pBackGround_;
private:
	Step step_;

private://wait
	float waitTime_;
	float kWaitTime_;
private:/// goup
	Easing goUpTime_;
private:/// apearUFO
	Easing apearUFO_;
private:/// aimWait
	float aimWaitTime_;
	float aimKWaitTime_;
private:/// aim
	float aimPosX_;
	Easing aimEase_;
	std::unique_ptr<Sprite> aimSprite_;
	float aimStartPos_;
	float aimEndPos_;
private:/// aimScale
	Easing kikAimEase_;
	std::unique_ptr<Sprite> kikAimSprite_;
	Vector2 kikAimScale_;
	const Vector2 kikTextureSize_ = {650,550};
	const Vector2 aimTextureSize_ = { 700.0f,150.0f };
private:/// CloseWait
	float closeWaitTime_;
	float closekWaitTime_;
	float aimCloseEndPos_;
private://aimclose
	float aimClosePosX_;
	Easing aimCloseEase_;
	float closeScale_;

	std::unique_ptr<Sprite> blackSprite_;
	bool isEnd_;
public:
	void Init();
	void Update();
	void Draw();
	void BlockSpriteDraw();
	void Debug();
	void OffsetMove();
	
	///==============================================================
	/// getter method
	///================================================================
	bool GetIsEnd()const { return isEnd_; }
	///================================================================
	/// setter method
	///================================================================
	
	void SetBackGround(BackGround* back);
	void SetSkyDome(SkyDome* skydome);
	void SetUFO(UFO* ufo);
};