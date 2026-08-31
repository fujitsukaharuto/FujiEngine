#include "ParticleDebugScene.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Core/Serialize/GlobalVariables.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Math/Random/Random.h"

#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Scene/SceneManager.h"
#include "Game/Particle/GameEmitters.h"
#include "Engine/Core/Input/Input.h"

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

	CheckSceneChange();

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

void ParticleDebugScene::CheckSceneChange() {
	// 暗転・明転中は受け付けない
	if (IsFading()) {
		return;
	}

	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		ChangeScene("TITLE");
	}
}
