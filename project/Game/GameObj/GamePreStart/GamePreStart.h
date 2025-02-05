
#pragma once

#include"Sprite.h"
#include"Easing.h"
#include"GlobalVariables/GlobalVariables.h"

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
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "PreGame";         /// グループ名


	struct Parameters {
		///wait
		
		float kWaitTime = 1.0f;
		/// goup
		float goUpTimeMax;
		/// apearUFO
		float apearUFOMax;
		/// aimWait	
		float aimKWaitTime = 1.0f;
		/// aim
		float aimEaseMax;
		float aimStartPos;
		float aimEndPos;
		/// CloseWait	
		float closekWaitTime = 1.0f;
		float aimCloseEndPos;
		//aimclose
		float aimClosePosX;
		float aimCloseEaseMax;
		float closeScale;
		///aimScale
		float kikAimEaseMax;
		Vector2 kikAimPos;
		Vector2 kikAimScale;
	};
private:
	Step step_;
	Parameters params_;
	std::array<float, 8>times_;

private:// aimScale
	std::unique_ptr<Sprite> aimSprite_;
	std::unique_ptr<Sprite> kikAimSprite_;

	const Vector2 kikTextureSize_ = { 650,550 };
	const Vector2 aimTextureSize_ = { 700.0f,150.0f };

	std::unique_ptr<Sprite> blackSprite_;
	bool isEnd_;
	float aimPosX;
	float closeScale_;
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

///-------------------------------------------------------------------------------------
/// Editor
///-------------------------------------------------------------------------------------
	void AdjustParm();
	void ParmLoadForImGui();
	void AddParmGroup();
	void SetValues();
	void ApplyGlobalParameter();

};