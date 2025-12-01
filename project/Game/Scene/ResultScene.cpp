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
#include "Engine/Editor/CommandManager.h"



ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	lightManager_->GetDirectionLight()->directionLightData_->direction = { 0.0f,-1.0f,0.0f };
	lightManager_->GetDirectionLight()->directionLightData_->intensity = 0.3f;
	ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).SetEmit(false);
}

void ResultScene::Initialize() {

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	CameraManager::GetInstance()->GetCamera()->transform.rotate = { cameraStartRotateX_,0.0f,0.0f };
	CameraManager::GetInstance()->GetCamera()->transform.translate = { 0.0f, 2.0f, -20.0f };
	lightManager_->GetDirectionLight()->directionLightData_->direction = lightDir_;
	lightManager_->GetDirectionLight()->directionLightData_->intensity = lightIntens_;

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

	terrain_ = std::make_unique<AnimationModel>();
	terrain_->Create("ground.obj");
	terrain_->IsMirrorOBJ(true);
	terrain_->SetEnvironmentCoeff(0.3f);
	terrain_->SetTexture("grass.jpg");
	terrain_->SetUVScale({ 20.0f,20.0f }, { 0.0f,0.0f });

	for (int i = 0; i < 3; i++) {
		std::unique_ptr<Object3d> player;
		player = std::make_unique<Object3d>();
		player->CreateFromJson("resource/Json/Clear_Player.json");
		player->SetTexture("Atlas.png");
		defoRotateY_ = player->transform.rotate.y;
		defoTransY_ = player->transform.translate.y;
		if (i == 1) {
			player->transform.translate.x += xDiff_;
			player->transform.translate.z += zDiff_;
		}
		else if (i == 2) {
			player->transform.translate.x -= xDiff_;
			player->transform.translate.z += zDiff_;
		}
		players_.push_back(std::move(player));
	}

	ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).Load("hanabi");
}

void ResultScene::Update() {

#ifdef _DEBUG


#endif // _DEBUG

	BlackFade();
	skybox_->Update();

	if (FPSKeeper::DeltaTime() < FPSKeeper::GetClampFrame()) {
		if (waitTime_ > 0.0f) {
			waitTime_ -= FPSKeeper::DeltaTime();
		} else {
			KirbyDance();
		}
	}

	ParticleManager::GetInstance()->Update();
}

void ResultScene::Draw() {

#pragma region 背景描画


	dxcommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	skybox_->Draw();

	obj3dCommon->PreDraw();
	terrain_->Draw();

	for (auto& player : players_) {
		player->Draw();
	}

	clear_->Draw();

#ifdef _DEBUG
	CommandManager::GetInstance()->Draw();
#endif // _DEBUG

	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxcommon_->PreSpriteDraw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion
	ModelManager::GetInstance()->PickingDataCopy();
}

void ResultScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	/*if (ImGui::CollapsingHeader("Sphere")) {
		ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
		ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	}*/

	ImGui::Unindent();
#endif // _DEBUG
}

void ResultScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	if (ImGui::Button("ResetDance")) {
		state_ = DanceState::TurnLeftMoveToLeft;
		danceTime_ = 0.0f;
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void ResultScene::BlackFade() {
	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		} else {
			ChangeScene("TITLE", 40.0f);
		}
	} else {
		if (blackTime > 0.0f) {
			blackTime -= FPSKeeper::DeltaTime();
			if (blackTime <= 0.0f) {
				blackTime = 0.0f;
			}
		}
	}
	black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimmite * blackTime)) });
	XINPUT_STATE pad;
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) && state_ == DanceState::Finish) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	} else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A) && state_ == DanceState::Finish) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
}

void ResultScene::ApplyGlobalVariables() {


}

void ResultScene::KirbyDance() {

	Vector3 transform = {};
	transform.y = defoTransY_;
	Vector3 rotate = {};
	rotate.y = defoRotateY_;
	danceTime_ += FPSKeeper::DeltaTime();
	float t = 0.0f;
	switch (state_) {
	case DanceState::TurnLeftMoveToLeft:
		t = danceTime_ / stepTime_.turnLeftBaseTime;
		transform.x = Lerp(0.0f, -danceDistanceX_, t);
		rotate.y += std::numbers::pi_v<float> * 2.0f * t;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::TurnRightMoveToCenter; }
		break;

	case DanceState::TurnRightMoveToCenter:
		t = danceTime_ / stepTime_.turnRightBaseTime;
		transform.x = Lerp(-danceDistanceX_, 0.0f, t);
		transform.y += hight_.jumpHeight * 4.0f * t * (1.0f - t);
		rotate.z = -std::numbers::pi_v<float> * 2.0f * t;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::JumpLeft; }
		break;

	case DanceState::JumpLeft:
		t = danceTime_ / stepTime_.jumpLeftBaseTime;
		transform.x = -1.0f * std::sin(t * std::numbers::pi_v<float>);
		transform.y += hight_.jumpHeight * 4.0f * t * (1.0f - t);
		rotate.z = jumpRotateZ_;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::JumpRight; }
		break;

	case DanceState::JumpRight:
		t = danceTime_ / stepTime_.jumpRightBaseTime;
		transform.x = 1.0f * sin(t * std::numbers::pi_v<float>);
		transform.y += hight_.jumpHeight * 4.0f * t * (1.0f - t);
		rotate.z = -jumpRotateZ_;

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::JumpUPSpin; }
		break;

	case DanceState::JumpUPSpin:
		t = danceTime_ / stepTime_.jumpUpBaseTime;
		rotate.y += std::numbers::pi_v<float> *2.0f * t;
		transform.y += hight_.finishHeight * std::sin(t * std::numbers::pi_v<float>);

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::FastSpin; }
		break;

	case DanceState::FastSpin:
		t = danceTime_ / stepTime_.fastSpinBaseTime;
		rotate.x = std::numbers::pi_v<float> * 2.0f * t;
		transform.y += hight_.spinHeight * std::sin(t * std::numbers::pi_v<float>);

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::FinishSpin; }
		break;
	case DanceState::FinishSpin:
		t = danceTime_ / stepTime_.finishSpinBaseTime;
		rotate.x = std::numbers::pi_v<float> * 2.0f *t;
		transform.y += hight_.finishHeight * std::sin(t * std::numbers::pi_v<float>);

		if (t >= 1.0f) { danceTime_ = 0.0f; state_ = DanceState::LastPose; }
		break;

	case DanceState::LastPose:
		t = danceTime_ / stepTime_.lastBaseTime;
		transform.y += hight_.lastHeight * std::sin(t * std::numbers::pi_v<float>);
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
		players_[i]->transform.translate = transform;
		players_[i]->transform.rotate = rotate;
		if (i == 1) {
			players_[i]->transform.translate.x += xDiff_;
			players_[i]->transform.translate.z += zDiff_;
		} else if (i == 2) {
			players_[i]->transform.translate.x -= xDiff_;
			players_[i]->transform.translate.z += zDiff_;
		}
	}
}

void ResultScene::HanabiUpdate() {
	Vector3 popPos = Random::GetVector3(popPos_.xMinMax, popPos_.yMinMax, popPos_.zMinMax);
	ParticleManager::GetInstance()->GetParticleCSEmitter(hanabiIndex_).SetPos(popPos);
}
