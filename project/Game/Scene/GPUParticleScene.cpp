#include "GPUParticleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Math/Random/Random.h"

#include "Engine/Model/Line3dDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


GPUParticleScene::GPUParticleScene() {}

GPUParticleScene::~GPUParticleScene() {
	FPSKeeper::SetUnStopped();
	ParticleManager::SetIsStopped(false);
	ParticleManager::GetInstance()->ResetCSEmitters();
	ParticleManager::GetInstance()->InitDefaultCSEmitter();
}

void GPUParticleScene::Initialize() {

	obj3dCommon_ = std::make_unique<Object3dCommon>();
	obj3dCommon_->Initialize();

	CameraManager::GetInstance()->GetCamera()->GetTransform().rotate = { 0.0f,0.0f,0.0f };

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor(Colors::Black);
	black_->SetSize({ MyWin::kWindowWidth,MyWin::kWindowHeight });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
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

}

void GPUParticleScene::Update() {

	skybox_->Update();

	BlackFade();

	ParticleManager::GetInstance()->Update();
}

void GPUParticleScene::Draw() {

#pragma region 背景描画


	dxcommon_->ClearDepthBuffer();
#pragma endregion

	skybox_->Draw();

#pragma region 3Dオブジェクト
	obj3dCommon_->PreDraw();
	terrain_->Draw();

	ParticleManager::GetInstance()->Draw();


	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxcommon_->PreSpriteDraw();
	if (blackTime_ != 0.0f) {
		black_->Draw();
	}

#pragma endregion
	ModelManager::GetInstance()->PickingDataCopy();
}

void GPUParticleScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void GPUParticleScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	ImGui::Unindent();
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
