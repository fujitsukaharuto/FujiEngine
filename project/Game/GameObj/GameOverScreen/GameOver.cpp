#include"GameOver.h"
#include"Input/Input.h"
#include"FPSKeeper.h"
#include<imgui.h>

void GameOver::Init() {
	overPaneru_ = std::make_unique<Sprite>();
	overPaneru_->Load("gameover.png");
	overPaneru_->SetAnchor({ 0.0f,0.0f });

	selectArrow_ = std::make_unique<Sprite>();
	selectArrow_->Load("GameTexture/Life.png");
	selectArrow_->SetAnchor({ 0.0f,0.0f });

	fadeOutEaseT_.maxTime = 0.5f;
	fadeOutEaseT_.time = 0.0f;
	fadeInEaseT_.maxTime = 0.5f;
	fadeInEaseT_.time = 0.0f;
	alpha_ = 0.0f;
	isGameBack_ = false;
	isTitleBack_ = false;
	
	overPaneru_->SetColor(Vector4(1, 1, 1, alpha_));
	selectArrow_->SetColor(Vector4(1, 1, 1, alpha_));
	
	selectMode_ = SelectMode::GOGAME;

	step_ = Step::FADEIN;
}

void GameOver::Update() {

	//alpha
	overPaneru_->SetColor(Vector4(1,1,1,alpha_));
	selectArrow_->SetColor(Vector4(1, 1, 1, alpha_));
	selectArrow_->SetPos(selectPos_);

	switch (step_)
	{
		///---------------------------------------------------------------
		/// フェードイン
		///---------------------------------------------------------------
	case GameOver::Step::FADEIN:
		fadeInEaseT_.time += FPSKeeper::NormalDeltaTime();
		alpha_ = EaseInCubic(0.0f, 1.0f, fadeInEaseT_.time, fadeInEaseT_.maxTime);

		if (fadeInEaseT_.time < fadeInEaseT_.maxTime)break;
		alpha_ = 1.0f;
		fadeInEaseT_.time = fadeInEaseT_.maxTime;
		step_ = Step::UISELECT;
		break;
		///---------------------------------------------------------------
		/// タイトルに戻るかゲームに戻るか
		///---------------------------------------------------------------
	case GameOver::Step::UISELECT:
		
		if (Input::GetInstance()->TriggerKey(DIK_S)&& selectMode_ == SelectMode::GOGAME) {
			selectMode_ = SelectMode::GOTITLE;
		}
		else if(Input::GetInstance()->TriggerKey(DIK_W) && selectMode_ == SelectMode::GOTITLE){
			selectMode_ = SelectMode::GOGAME;
		}

		///　決定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {


			//ゲームに戻る
			if (selectMode_ == SelectMode::GOGAME) {
				step_ = Step::FADEOUT;
				break;
			}

			//タイトルに戻るなら
			if (selectMode_ == SelectMode::GOTITLE) {
				step_ = Step::GOTITLE;
				break;
			}
		}
		break;
		///---------------------------------------------------------------
		/// タイトルに戻るフラグを立てる
		///---------------------------------------------------------------
	case GameOver::Step::GOTITLE:
		isTitleBack_ = true;
		break;
		///---------------------------------------------------------------
		/// フェードアウト
		///---------------------------------------------------------------
	case GameOver::Step::FADEOUT:
		fadeOutEaseT_.time += FPSKeeper::NormalDeltaTime();
		alpha_ = EaseInCubic(1.0f, 0.0f, fadeOutEaseT_.time, fadeOutEaseT_.maxTime);

		if (fadeOutEaseT_.time < fadeOutEaseT_.maxTime)break;
		alpha_ = 0.0f;
		fadeOutEaseT_.time = fadeOutEaseT_.maxTime;
		isGameBack_ = true;
		break;
	default:
		break;
	}
}

void GameOver::Draw() {
	overPaneru_->Draw();
	selectArrow_->Draw();
}

void GameOver::Debug() {
	ImGui::DragFloat3("selectPos_", &selectPos_.x, 0.1f);
}