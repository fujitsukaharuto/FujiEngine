#include "ResultScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Math/Random/Random.h"
#include <numbers>

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"
#include "Engine/Light/LightManager.h"
#include "Engine/Model/Sprite/SpriteRenderer.h"
#include "Engine/Editor/CommandManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Editor;
using namespace Scene;
using namespace DXC;


ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	lightManager_->GetDirectionLight()->SetLightDirection(Vector3::Down());
	lightManager_->GetDirectionLight()->SetLightIntensity(0.3f);
	ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).SetEmit(false);
	ParticleManager::GetInstance()->ResetCSEmitters();
	ParticleManager::GetInstance()->InitDefaultCSEmitter();
}

void ResultScene::Initialize() {

	CameraManager::GetInstance()->GetCamera()->GetTransform().rotate = { cameraStartRotateX_,0.0f,0.0f };
	CameraManager::GetInstance()->GetCamera()->GetTransform().translate = cameraPos_;
	lightManager_->GetDirectionLight()->SetLightDirection(lightDir_);
	lightManager_->GetDirectionLight()->SetLightIntensity(lightIntensity_);

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	clear_ = std::make_unique<Sprite>();
	clear_->Load("clear_beta.png");
	clear_->SetAnchor({ 0.0f,0.0f });
	clear_->SetSize({ 1280.0f,720.0f });

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();
	skybox_->SetColor(skyBoxColor_);

	terrain_ = std::make_unique<AnimationModel>();
	terrain_->Create("ground.obj");
	terrain_->IsMirrorOBJ(true);
	terrain_->SetEnvironmentCoeff(0.3f);
	terrain_->SetTexture("grass.jpg");
	terrain_->SetColor(terrainColor_);

	surroundings_ = std::make_unique<Object3d>();
	surroundings_->Create("surroundings.gltf");
	surroundings_->LoadTransformFromJson("surroundings_transform.json");
	surroundings_->SetColor(surroundingColor_);
	surroundings_->SetLightEnable(LightMode::kSpotLightON);

	for (int i = 0; i < 3; i++) {
		std::unique_ptr<Object3d> player;
		player = std::make_unique<Object3d>();
		player->CreateFromJson("resource/Json/Clear_Player.json");
		player->SetTexture("Atlas.png");
		defaRotateY_ = player->GetTransform().rotate.y;
		defaTransY_ = player->GetTransform().translate.y;
		if (i == 1) {
			player->GetTransform().translate.x += xDiff_;
			player->GetTransform().translate.z += zDiff_;
		}
		else if (i == 2) {
			player->GetTransform().translate.x -= xDiff_;
			player->GetTransform().translate.z += zDiff_;
		}
		players_.push_back(std::move(player));
	}

	ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).Load("hanabi");
}

void ResultScene::Update() {

#ifdef _DEBUGMODE


#endif // _DEBUG

	BlackFade();

	if (FPSKeeper::DeltaTimeFrame() < FPSKeeper::GetClampFrame()) {
		if (waitTime_ > 0.0f) {
			waitTime_ -= FPSKeeper::DeltaTimeFrame();
		} else {
			KirbyDance();
		}
	}

}

void ResultScene::Draw() {

#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	skybox_->Draw();

	terrain_->Draw();
	surroundings_->Draw();

	for (auto& player : players_) {
		player->Draw();
	}

	clear_->Draw();

#ifdef _DEBUGMODE
	CommandManager::GetInstance()->Draw();
#endif // _DEBUG

#pragma endregion

#pragma region 前景スプライト
	if (blackTime_ != 0.0f) {
		black_->Draw();
	}

#pragma endregion
}

void ResultScene::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();



	ImGui::Unindent();
#endif // _DEBUG
}

void ResultScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	if (ImGui::Button("ResetDance")) {
		state_ = DanceState::TurnLeftMoveToLeft;
		danceTime_ = 0.0f;
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void ResultScene::BlackFade() {
	if (isChangePhase_) {
		if (blackTime_ < blackLimit_) {
			blackTime_ += FPSKeeper::DeltaTimeFrame();
			if (blackTime_ >= blackLimit_) {
				blackTime_ = blackLimit_;
			}
		} else {
			ChangeScene("TITLE", 40.0f);
		}
	} else {
		if (blackTime_ > 0.0f) {
			blackTime_ -= FPSKeeper::DeltaTimeFrame();
			if (blackTime_ <= 0.0f) {
				blackTime_ = 0.0f;
			}
		}
	}
	black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimit_ * blackTime_)) });
	XINPUT_STATE pad;
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) && state_ == DanceState::Finish) {
		if (blackTime_ == 0.0f) {
			isChangePhase_ = true;
		}
	} else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A) && state_ == DanceState::Finish) {
			if (blackTime_ == 0.0f) {
				isChangePhase_ = true;
			}
		}
	}
}

void ResultScene::ApplyGlobalVariables() {


}

void ResultScene::KirbyDance() {

	Vector3 transform = {};
	transform.y = defaTransY_;
	Vector3 rotate = {};
	rotate.y = defaRotateY_;
	danceTime_ += FPSKeeper::DeltaTimeFrame();
	float t = 0.0f;
	switch (state_) {
	case DanceState::TurnLeftMoveToLeft:// 左に動く
		t = danceTime_ / stepTime_.turnLeftBaseTime;
		transform.x = Lerp(0.0f, -danceDistanceX_, t);
		rotate.y += std::numbers::pi_v<float> * 2.0f * t;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::TurnRightMoveToCenter; }
		break;

	case DanceState::TurnRightMoveToCenter:// 真ん中に戻る
		t = danceTime_ / stepTime_.turnRightBaseTime;
		transform.x = Lerp(-danceDistanceX_, 0.0f, t);
		transform.y += height_.jumpHeight * 4.0f * t * (1.0f - t);
		rotate.z = -std::numbers::pi_v<float> * 2.0f * t;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::JumpLeft; }
		break;

	case DanceState::JumpLeft:// 左にジャンプ
		t = danceTime_ / stepTime_.jumpLeftBaseTime;
		transform.x = -1.0f * std::sin(t * std::numbers::pi_v<float>);
		transform.y += height_.jumpHeight * 4.0f * t * (1.0f - t);
		rotate.z = jumpRotateZ_;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::JumpRight; }
		break;

	case DanceState::JumpRight:// 右にジャンプ
		t = danceTime_ / stepTime_.jumpRightBaseTime;
		transform.x = 1.0f * sin(t * std::numbers::pi_v<float>);
		transform.y += height_.jumpHeight * 4.0f * t * (1.0f - t);
		rotate.z = -jumpRotateZ_;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::JumpUPSpin; }
		break;

	case DanceState::JumpUPSpin:// ジャンプして回転
		t = danceTime_ / stepTime_.jumpUpBaseTime;
		rotate.y += std::numbers::pi_v<float> *2.0f * t;
		transform.y += height_.finishHeight * std::sin(t * std::numbers::pi_v<float>);

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::FastSpin; }
		break;

	case DanceState::FastSpin:// 素早く回転
		t = danceTime_ / stepTime_.fastSpinBaseTime;
		rotate.x = std::numbers::pi_v<float> * 2.0f * t;
		transform.y += height_.spinHeight * std::sin(t * std::numbers::pi_v<float>);

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::FinishSpin; }
		break;
	case DanceState::FinishSpin:// 最後の回転
		t = danceTime_ / stepTime_.finishSpinBaseTime;
		rotate.x = std::numbers::pi_v<float> * 2.0f *t;
		transform.y += height_.finishHeight * std::sin(t * std::numbers::pi_v<float>);

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::LastPose; }
		break;

	case DanceState::LastPose:// 決めポーズ
		t = danceTime_ / stepTime_.lastBaseTime;
		transform.y += height_.lastHeight * std::sin(t * std::numbers::pi_v<float>);
		rotate.y -= lastRotateY_ * t;  // Y軸：右に20°
		rotate.x = -lastRotateX_ * t;

		if (t >= 1.0f) {
			danceTime_ = 0.0f;
			state_ = DanceState::Finish;
			ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).SetEmit(true);
			ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).Emit();
		}
		break;

	case DanceState::Finish:
		rotate.y -= lastRotateY_;
		rotate.x = -lastRotateX_;
		HanabiUpdate();
		break;
	}

	for (int i = 0; i < 3; i++) {
		players_[i]->GetTransform().translate = transform;
		players_[i]->GetTransform().rotate = rotate;
		if (i == 1) {
			players_[i]->GetTransform().translate.x += xDiff_;
			players_[i]->GetTransform().translate.z += zDiff_;
		} else if (i == 2) {
			players_[i]->GetTransform().translate.x -= xDiff_;
			players_[i]->GetTransform().translate.z += zDiff_;
		}
	}
}

void ResultScene::HanabiUpdate() {
	Vector3 popPos = Random::GetVector3(popPos_.xMinMax, popPos_.yMinMax, popPos_.zMinMax);
	ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).SetPos(popPos);
}
