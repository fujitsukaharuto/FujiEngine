#include "GPUParticleScene.h"
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


GPUParticleScene::GPUParticleScene() {}

GPUParticleScene::~GPUParticleScene() {
	FPSKeeper::SetUnStopped();
	ParticleManager::SetIsStopped(false);
	ParticleManager::GetInstance()->ResetCSEmitters();
	Game::CreateDefaultEmitters();
}

void GPUParticleScene::Initialize() {

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
	terrain_->GetTransform().translate.y = terrainPosY_;
	terrain_->SetEnvironmentCoeff(terrainEnvironmentCoeff_);
	terrain_->SetTexture("grass.jpg");
	terrain_->SetColor(terrainColor_);

	int emitNum = ParticleManager::GetInstance()->InitGPUEmitter();
	auto& emitterCS = ParticleManager::GetSphereEmitter(emitNum);
	emitterCS.Load("bossHandAura");
	emitterCS.SetEmit(true);

	// メッシュ表面エミッターの実験用
	ParticleManager::GetInstance()->InitGPUEmitterSurface("Terrain_1783481558545.obj");
	ParticleManager::GetInstance()->InitGPUEmitterSurface("Terrain_1783481558545.obj");
	ParticleManager::GetInstance()->InitGPUEmitterSurface("Terrain_1783481558545.obj");

}

void GPUParticleScene::Update() {

	BlackFade();

}

void GPUParticleScene::Draw() {
#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	skybox_->Draw();

	terrain_->Draw();

#pragma endregion

#pragma region 前景スプライト
	if (blackTime_ != 0.0f) {
		black_->Draw();
	}

#pragma endregion
}

void GPUParticleScene::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void GPUParticleScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void GPUParticleScene::ParticleGroupDebugGUI() {
#ifdef _DEBUGMODE
	ParticleManager::GetInstance()->GPUParticleSceneGUI();
#endif // _DEBUG
}

void GPUParticleScene::BlackFade() {
	if (isChangePhase_) {
		if (blackTime_ < blackLimit_) {
			blackTime_ += FPSKeeper::DeltaTimeFrame();
			if (blackTime_ >= blackLimit_) {
				blackTime_ = blackLimit_;
			}
		} else {
			ChangeScene("GPUPARTICLE", 40.0f);
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

	if (Input::GetInstance()->PushKey(DIK_LSHIFT) && Input::GetInstance()->TriggerKey(DIK_R)) {
		if (blackTime_ == 0.0f) {
			isChangePhase_ = true;// Sceneをリセットするために
		}
	}

}
