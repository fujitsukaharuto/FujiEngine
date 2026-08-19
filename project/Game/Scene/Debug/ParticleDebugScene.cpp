#include "ParticleDebugScene.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Core/Serialize/GlobalVariables.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Math/Random/Random.h"

#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Graphics/Sprite/SpriteRenderer.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Game/Particle/GameEmitters.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Scene;
using namespace DXC;


ParticleDebugScene::ParticleDebugScene() {}

ParticleDebugScene::~ParticleDebugScene() {
	FPSKeeper::SetUnStopped();
	ParticleManager::SetIsStopped(false);
	ParticleManager::GetInstance()->ResetCSEmitters();
	Game::CreateDefaultEmitters();
}

void ParticleDebugScene::Initialize() {

	CameraManager::GetInstance()->GetCamera()->GetTransform().rotate = { 0.0f,0.0f,0.0f };

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor(Colors::Black);
	black_->SetSize({ MyWin::kWindowWidth,MyWin::kWindowHeight });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	skybox_ = std::make_unique<SkyBox>();
	skybox_->Initialize();
	skybox_->SetColor(skyBoxColor_);

	terrain_ = std::make_unique<AnimationModel>();
	terrain_->Create("ground.obj");
	terrain_->IsMirrorOBJ(true);
	terrain_->GetTransform().translate.y = -5.0f;
	terrain_->SetEnvironmentCoeff(0.3f);
	terrain_->SetTexture("grass.jpg");
	terrain_->SetColor(terrainColor_);

}

void ParticleDebugScene::Update() {

#ifdef _DEBUGMODE

	ParticleManager::GetInstance()->SelectParticleUpdate();

#endif // _DEBUG

	BlackFade();

}

void ParticleDebugScene::Draw() {
#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	skybox_->Draw();
	terrain_->Draw();

#ifdef _DEBUGMODE
	ParticleManager::GetInstance()->SelectEmitterSizeDraw();
#endif // _DEBUG

#pragma endregion

#pragma region 前景スプライト
	if (blackTime_ != 0.0f) {
		black_->Draw();
	}

#pragma endregion
}

void ParticleDebugScene::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void ParticleDebugScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void ParticleDebugScene::ParticleGroupDebugGUI() {
#ifdef _DEBUGMODE
	ParticleManager::GetInstance()->ParticleDebugGUI();
#endif // _DEBUG
}

void ParticleDebugScene::BlackFade() {
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
	
	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		if (blackTime_ == 0.0f) {
			isChangePhase_ = true;
		}
	}

}
