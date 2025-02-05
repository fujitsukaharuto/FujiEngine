#include"GameOver.h"
#include"Input/Input.h"
#include"FPSKeeper.h"
#include<imgui.h>

void GameOver::Init() {
	overPaneru_ = std::make_unique<Sprite>();
	overPaneru_->Load("GameTexture/gameover.png");
	overPaneru_->SetAnchor({ 0.0f,0.0f });

	selectArrow_ = std::make_unique<Sprite>();
	selectArrow_->Load("GameTexture/GO_Serect.png");
	selectArrow_->SetAnchor({ 0.0f,0.0f });

	///parm初期化
	fadeOutEaseT_.maxTime = 0.5f;
	fadeOutEaseT_.time = 0.0f;
	fadeInEaseT_.maxTime = 0.5f;
	fadeInEaseT_.time = 0.0f;
	alpha_ = 0.0f;
	isGameBack_ = false;
	isTitleBack_ = false;

	overPaneru_->SetColor(Vector4(1, 1, 1, alpha_));
	selectArrow_->SetColor(Vector4(1, 1, 1, alpha_));

	selectPos_ = gameSelectPos_;
	easeDirection_ = 1.0f;
	selectPosOffSet_ = 0.0f;
	offsetEasing_.maxTime = 0.4f;
	offsetEasing_.time = 0.0f;

	dicisionSE_ = Audio::GetInstance()->SoundLoadWave("DecisionSE.wav");
	selectSE_ = Audio::GetInstance()->SoundLoadWave("missileSize.wav");

	selectMode_ = SelectMode::GOGAME;
	step_ = Step::FADEIN;
}

//void GameOver::SetStepStart() {
//	step_ = Step::FADEIN;
//}

void GameOver::Update() {

	//alpha
	overPaneru_->SetColor(Vector4(1, 1, 1, alpha_));
	selectArrow_->SetColor(Vector4(1, 1, 1, alpha_));
	selectArrow_->SetPos(Vector3(selectPos_.x + selectPosOffSet_, selectPos_.y, selectPos_.z));

	switch (step_)
	{
		///---------------------------------------------------------------
		/// フェードイン
		///---------------------------------------------------------------
	case GameOver::Step::FADEIN:
		fadeInEaseT_.time += FPSKeeper::NormalDeltaTime();
		alpha_ = EaseOutCubic(0.0f, 1.0f, fadeInEaseT_.time, fadeInEaseT_.maxTime);

		if (fadeInEaseT_.time < fadeInEaseT_.maxTime)break;
		alpha_ = 1.0f;
		fadeInEaseT_.time = fadeInEaseT_.maxTime;
		step_ = Step::UISELECT;
		break;
		///---------------------------------------------------------------
		/// タイトルに戻るかゲームに戻るか
		///---------------------------------------------------------------
	case GameOver::Step::UISELECT:

		OffsetMove();

		if (Input::GetInstance()->TriggerKey(DIK_S) && selectMode_ == SelectMode::GOGAME) {
			selectMode_ = SelectMode::GOTITLE;
			selectPos_ = titleSelectPos_;
			Audio::GetInstance()->SoundPlayWave(selectSE_);
		} else if (Input::GetInstance()->TriggerKey(DIK_W) && selectMode_ == SelectMode::GOTITLE) {
			selectMode_ = SelectMode::GOGAME;
			selectPos_ = gameSelectPos_;
			Audio::GetInstance()->SoundPlayWave(selectSE_);
		}

		XINPUT_STATE pad;
		if (Input::GetInstance()->GetGamepadState(pad)) {
			bool ismoving = false;
			const float thresholdValue = 0.8f;
			Vector2 stickVelocity;
			stickVelocity = { 0.0f,Input::GetInstance()->GetLStick().y / SHRT_MAX };
			if (stickVelocity.Length() > thresholdValue) {
				ismoving = true;
			}

			if ((Input::GetInstance()->TriggerButton(PadInput::Down) || (ismoving && stickVelocity.y < 0.0f))
				&& selectMode_ == SelectMode::GOGAME) {
				selectMode_ = SelectMode::GOTITLE;
				selectPos_ = titleSelectPos_;
				Audio::GetInstance()->SoundPlayWave(selectSE_);
			} else if ((Input::GetInstance()->TriggerButton(PadInput::Up) || (ismoving && stickVelocity.y > 0.0f))
				&& selectMode_ == SelectMode::GOTITLE) {
				selectMode_ = SelectMode::GOGAME;
				selectPos_ = gameSelectPos_;
				Audio::GetInstance()->SoundPlayWave(selectSE_);
			}
		}

		///　決定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {


			//ゲームに戻る
			if (selectMode_ == SelectMode::GOGAME) {
				step_ = Step::FADEOUT;
				Audio::GetInstance()->SoundPlayWave(dicisionSE_);
				break;
			}

			//タイトルに戻るなら
			if (selectMode_ == SelectMode::GOTITLE) {
				step_ = Step::GOTITLE;
				Audio::GetInstance()->SoundPlayWave(dicisionSE_);
				break;
			}
		}
		if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::A)) {
				//ゲームに戻る
				if (selectMode_ == SelectMode::GOGAME) {
					step_ = Step::FADEOUT;
					Audio::GetInstance()->SoundPlayWave(dicisionSE_);
					break;
				}

				//タイトルに戻るなら
				if (selectMode_ == SelectMode::GOTITLE) {
					step_ = Step::GOTITLE;
					Audio::GetInstance()->SoundPlayWave(dicisionSE_);
					break;
				}
			}
		}

		break;
		///---------------------------------------------------------------
		/// タイトルに戻るフラグを立てる
		///---------------------------------------------------------------
	case GameOver::Step::GOTITLE:
		OffsetMove();
		isTitleBack_ = true;
		break;
		///---------------------------------------------------------------
		/// フェードアウト
		///---------------------------------------------------------------
	case GameOver::Step::FADEOUT:
		OffsetMove();

		fadeOutEaseT_.time += FPSKeeper::NormalDeltaTime();
		alpha_ = EaseOutCubic(1.0f, 0.0f, fadeOutEaseT_.time, fadeOutEaseT_.maxTime);


		if (fadeOutEaseT_.time < fadeOutEaseT_.maxTime)break;
		alpha_ = 0.0f;
		overPaneru_->SetColor(Vector4(1, 1, 1, alpha_));
		selectArrow_->SetColor(Vector4(1, 1, 1, alpha_));
		fadeOutEaseT_.time = fadeOutEaseT_.maxTime;
		isGameBack_ = true;
		break;
	default:
		break;
	}
}

void GameOver::Draw() {
	if (alpha_ <= 0.0f)return;
	overPaneru_->Draw();
	selectArrow_->Draw();
}

void GameOver::Debug() {
	ImGui::DragFloat3("selectPos_", &selectPos_.x, 0.1f);
}


void GameOver::OffsetMove() {
	// イージングタイムを更新
	offsetEasing_.time += FPSKeeper::NormalDeltaTime() * easeDirection_; // 方向に応じて時間を増減

	// タイムが1を超えたら逆方向に、0未満になったら進む方向に変更
	if (offsetEasing_.time >= offsetEasing_.maxTime) {
		offsetEasing_.time = offsetEasing_.maxTime;
		easeDirection_ = -1.0f; // 逆方向に切り替え
	} else if (offsetEasing_.time <= 0.0f) {
		offsetEasing_.time = 0.0f;
		easeDirection_ = 1.0f; // 進む方向に切り替え
	}

	selectPosOffSet_ = EaseInCubic(-8.0f,11.0f, offsetEasing_.time, offsetEasing_.maxTime);

}