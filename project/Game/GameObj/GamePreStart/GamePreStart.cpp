#include"GamePreStart.h"
#include"GameObj/UFO/UFO.h"
#include"GameObj/SkyDome/SkyDome.h"
#include"FPSKeeper.h"
#include<imgui.h>

void GamePreStart::Init() {
	aimSprite_ = std::make_unique<Sprite>();
	aimSprite_->Load("GameTexture/Life.png");
	aimSprite_->SetAnchor({ 0.0f,0.0f });

	// wait
	waitTime_ = 0.0f;
	kWaitTime_ = 1.0f;
	aimWaitTime_ = 0.0f;
	aimKWaitTime_ = 1.0f;
	closeWaitTime_ = 0.0f;
	closekWaitTime_ = 1.0f;

	aimEase_.maxTime = 1.5f;
	aimCloseEase_.maxTime = 1.0f;
	goUpTime_.maxTime = 1.0f;

	aimStartPos_ = -1280.0f;
	aimEndPos_ = 0.0f;
	aimCloseEndPos_ = 1280.0f;
	aimPosX_ = aimStartPos_;

	aimSprite_->SetPos(Vector3(aimStartPos_, 320.0f, 0));

	step_ = Step::WAIT;
}

//void GameOver::SetStepStart() {
//	step_ = Step::FADEIN;
//}

void GamePreStart::Update() {

	aimSprite_->SetPos(Vector3(aimPosX_, 320.0f, 0));

	switch (step_)
	{
		///-----------------------------------------------------------------
		///最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::WAIT:
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < kWaitTime_)break;
		waitTime_ = kWaitTime_;
		step_ = Step::GOUP;
		break;
		///-----------------------------------------------------------------
		///上に上がる
		///-----------------------------------------------------------------
	case GamePreStart::Step::GOUP:
		goUpTime_.time += FPSKeeper::NormalDeltaTime();
		pUFO_->Apear(goUpTime_.time, goUpTime_.maxTime);
		pSkyDome_->Apear(goUpTime_.time, goUpTime_.maxTime);

		if (goUpTime_.time < goUpTime_.maxTime)break;
		goUpTime_.time = goUpTime_.maxTime;
		step_ = Step::AIMWAIT;
		break;
		///-----------------------------------------------------------------
		///最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMWAIT:
		aimWaitTime_ += FPSKeeper::NormalDeltaTime();
		if (aimWaitTime_ < aimKWaitTime_)break;
		aimWaitTime_ = aimKWaitTime_;
		step_ = Step::AIMOPEN;
		break;
		///-----------------------------------------------------------------
		///最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMOPEN:
		aimEase_.time += FPSKeeper::NormalDeltaTime();
		aimPosX_ = EaseOutCubic(aimStartPos_, aimEndPos_, aimEase_.time, aimEase_.maxTime);

		if (aimEase_.time < aimEase_.maxTime)break;	
		aimEase_.time = aimEase_.maxTime;
		aimPosX_ = aimEndPos_;
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
		aimCloseEase_.time += FPSKeeper::NormalDeltaTime();
		aimClosePosX_ = EaseOutCubic(aimEndPos_, aimCloseEndPos_, aimCloseEase_.time, aimCloseEase_.maxTime);

		if (aimCloseEase_.time < aimEase_.maxTime)break;
		aimPosX_ = aimCloseEndPos_;
		aimCloseEase_.time = aimEase_.maxTime;
		isEnd_ = true;
		break;
	default:
		break;
	}
}

void GamePreStart::Draw() {
	
	aimSprite_->Draw();

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