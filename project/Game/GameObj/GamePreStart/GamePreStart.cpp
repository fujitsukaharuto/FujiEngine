#include"GamePreStart.h"
#include"GameObj/UFO/UFO.h"
#include"GameObj/SkyDome/SkyDome.h"
#include"GameObj/BackGround/BackGround.h"
#include"FPSKeeper.h"
#include<imgui.h>

void GamePreStart::Init() {
	aimSprite_ = std::make_unique<Sprite>();
	aimSprite_->Load("GameTexture/StartUI.png");
	aimSprite_->SetAnchor({ 0.0f,0.0f });

	kikAimSprite_ = std::make_unique<Sprite>();
	kikAimSprite_->Load("GameTexture/StartUI_Kick.png");
	kikAimSprite_->SetAnchor({ 0.5f,0.5f });

	blackSprite_ = std::make_unique<Sprite>();
	blackSprite_->Load("white2x2.png");
	blackSprite_->SetColor({ 0.0f,0.0f,0.0f,0.5f });
	blackSprite_->SetSize({ 1280.0f,950.0f });
	blackSprite_->SetAnchor({ 0.0f,0.0f });

	// wait
	waitTime_ = 0.0f;
	kWaitTime_ = 1.0f;
	aimWaitTime_ = 0.0f;
	aimKWaitTime_ = 1.0f;
	closeWaitTime_ = 0.0f;
	closekWaitTime_ = 1.0f;
	closeScale_= 1.0f;

	aimEase_.maxTime = 0.5f;
	kikAimEase_.maxTime = 0.5f;
	aimCloseEase_.maxTime = 1.0f;
	goUpTime_.maxTime = 1.5f;
	apearUFO_.maxTime = 0.6f;

	aimStartPos_ = 1200.0f;
	aimEndPos_ = 100.0f;
	aimCloseEndPos_ = -1280.0f;
	aimPosX_ = aimStartPos_;

	///pos
	aimSprite_->SetPos(Vector3(aimStartPos_, 210.0f, 0));
	kikAimSprite_->SetPos(Vector3(500.0f, 620.0f, 0));
	kikAimSprite_->SetSize(Vector2(0,0));

	step_ = Step::WAIT;
}

//void GameOver::SetStepStart() {
//	step_ = Step::FADEIN;
//}

void GamePreStart::Update() {

	aimSprite_->SetPos(Vector3(aimPosX_, 210.0f, 0));

	switch (step_)
	{
		///-----------------------------------------------------------------
		/// 最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::WAIT:
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < kWaitTime_)break;
		waitTime_ = kWaitTime_;
		step_ = Step::GOUPGROUND;
		break;

		///-----------------------------------------------------------------
		/// 上に上がる
		///-----------------------------------------------------------------
	case GamePreStart::Step::GOUPGROUND:
		goUpTime_.time += FPSKeeper::NormalDeltaTime();
		/// 出現
		pBackGround_->Scrool(goUpTime_.time, goUpTime_.maxTime);

		if (goUpTime_.time < goUpTime_.maxTime)break;
		goUpTime_.time = goUpTime_.maxTime;
		step_ = Step::APEARUFO;
		break;
		///-----------------------------------------------------------------
		/// 上に上がる
		///-----------------------------------------------------------------
	case GamePreStart::Step::APEARUFO:
		apearUFO_.time += FPSKeeper::NormalDeltaTime();
		/// 出現
		pUFO_->Apear(apearUFO_.time, apearUFO_.maxTime);
	
		if (apearUFO_.time < apearUFO_.maxTime)break;
		apearUFO_.time = apearUFO_.maxTime;
		step_ = Step::AIMWAIT;
		break;
		///-----------------------------------------------------------------
		/// 最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMWAIT:
		aimWaitTime_ += FPSKeeper::NormalDeltaTime();
		if (aimWaitTime_ < aimKWaitTime_)break;
		aimWaitTime_ = aimKWaitTime_;
		step_ = Step::AIMOPEN;
		break;
		///-----------------------------------------------------------------
		/// 最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMOPEN:
		aimEase_.time += FPSKeeper::NormalDeltaTime();
		aimPosX_ = EaseOutCubic(aimStartPos_, aimEndPos_, aimEase_.time, aimEase_.maxTime);

		if (aimEase_.time < aimEase_.maxTime)break;	
		aimEase_.time = aimEase_.maxTime;
		aimPosX_ = aimEndPos_;
		step_ = Step::AIMKIKOPEN;
	
		break;
	///-----------------------------------------------------------------
	/// キックエイム
	///-----------------------------------------------------------------
	case GamePreStart::Step::AIMKIKOPEN:
		//タイム
		kikAimEase_.time += FPSKeeper::NormalDeltaTime();
		kikAimScale_ = EaseOutCubic(Vector2(0,0), Vector2(1, 1), kikAimEase_.time, kikAimEase_.maxTime);

		///適応
		kikAimSprite_->SetSize(Vector2(kikTextureSize_.x * kikAimScale_.x, kikTextureSize_.y * kikAimScale_.y));

		if (kikAimEase_.time < kikAimEase_.maxTime)break;
		kikAimEase_.time = kikAimEase_.maxTime;
		kikAimSprite_->SetSize(kikTextureSize_);
		step_ = Step::CLOSEWAIT;

		break;
		///-----------------------------------------------------------------
		///閉め待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::CLOSEWAIT:
		closeWaitTime_ += FPSKeeper::NormalDeltaTime();
		if (closeWaitTime_ < closekWaitTime_)break;
		closeWaitTime_ = closekWaitTime_;
		step_ = Step::AIMCLOSE;
		break;
		///-----------------------------------------------------------------
		///閉め
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMCLOSE:
		// タイム
		aimCloseEase_.time += FPSKeeper::NormalDeltaTime();
		closeScale_ = EaseInBack(1.0f, 0.0f, aimCloseEase_.time, aimCloseEase_.maxTime);

		///適応
		kikAimSprite_->SetSize(Vector2(kikTextureSize_.x * closeScale_, kikTextureSize_.y * closeScale_));
		aimSprite_->SetSize(Vector2(aimTextureSize_.x * closeScale_, aimTextureSize_.y * closeScale_));

		/// 終了
		if (aimCloseEase_.time < aimEase_.maxTime)break;
		aimPosX_ = aimCloseEndPos_;
		aimCloseEase_.time = aimEase_.maxTime;
		kikAimSprite_->SetSize(Vector2(0, 0));
		aimSprite_->SetSize(Vector2(0, 0));
		//エンド
		isEnd_ = true;
		step_ = Step::END;
		break;
		///-----------------------------------------------------------------
		/// 終わり
		///-----------------------------------------------------------------
	case GamePreStart::Step::END:

		break;
	default:
		break;
	}
}

void GamePreStart::Draw() {
	aimSprite_->Draw();
	kikAimSprite_->Draw();
}

void  GamePreStart::BlockSpriteDraw() {
	if (step_ == Step::AIMOPEN    ||
		step_ == Step::AIMKIKOPEN ||
		step_ == Step::CLOSEWAIT  ||
		step_ == Step::AIMCLOSE) {
		blackSprite_->Draw();
	}
}

void GamePreStart::Debug() {
	//ImGui::DragFloat3("selectPos_", &selectPos_.x, 0.1f);
}


void GamePreStart::OffsetMove() {
	
}

void GamePreStart::SetSkyDome(SkyDome* skydome) {
	pSkyDome_ = skydome;
}
void GamePreStart::SetUFO(UFO* ufo) {
	pUFO_ = ufo;
}
void GamePreStart::SetBackGround(BackGround* back) {
	pBackGround_ = back;
}