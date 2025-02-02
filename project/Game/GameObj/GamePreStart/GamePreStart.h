#pragma once

#include"Sprite.h"
#include"Easing.h"

class UFO;
class SkyDome;
class GamePreStart {
public:
	enum class Step {
		WAIT,
		GOUP,
		AIMWAIT,
		AIMOPEN,
		CLOSEWAIT,
		AIMCLOSE,
	};

private:
	UFO* pUFO_;
	SkyDome* pSkyDome_;
private:
	Step step_;

private://wait
	float waitTime_;
	float kWaitTime_;
private:/// goup
	Easing goUpTime_;
private:/// aimWait
	float aimWaitTime_;
	float aimKWaitTime_;
private:/// aim
	float aimPosX_;
	Easing aimEase_;
	std::unique_ptr<Sprite> aimSprite_;
	float aimStartPos_;
	float aimEndPos_;
private:/// CloseWait
	float closeWaitTime_;
	float closekWaitTime_;
	float aimCloseEndPos_;
private://aimclose
	float aimClosePosX_;
	Easing aimCloseEase_;

	bool isEnd_;
public:
	void Init();
	void Update();
	void Draw();
	void Debug();
	void OffsetMove();
	
	///==============================================================
	/// getter method
	///================================================================
	bool GetIsEnd()const { return isEnd_; }
	///================================================================
	/// setter method
	///================================================================
	
	void SetSkyDome(SkyDome* skydome);
	void SetUFO(UFO* ufo);
};