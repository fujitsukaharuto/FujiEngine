#include "ParticleDebugScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Math/Random/Random.h"

#include "Engine/Model/Line3dDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"



ParticleDebugScene::ParticleDebugScene() {}

ParticleDebugScene::~ParticleDebugScene() {
}

void ParticleDebugScene::Initialize() {

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	CameraManager::GetInstance()->GetCamera()->transform.rotate = { 0.0f,0.0f,0.0f };

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();

}

void ParticleDebugScene::Update() {

	skybox_->Update();

#ifdef _DEBUG

	ParticleManager::GetInstance()->SelectParticleUpdate();

#endif // _DEBUG

	BlackFade();

	ParticleManager::GetInstance()->Update();
}

void ParticleDebugScene::Draw() {

#pragma region 背景描画


	dxcommon_->ClearDepthBuffer();
#pragma endregion

	skybox_->Draw();

#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();


	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG

	ParticleManager::GetInstance()->SelectEmitterSizeDraw();

#endif // _DEBUG

	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxcommon_->PreSpriteDraw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion
	ModelManager::GetInstance()->PickingDataCopy();
}

void ParticleDebugScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void ParticleDebugScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void ParticleDebugScene::ParticleGroupDebugGUI() {
#ifdef _DEBUG
	ParticleManager::GetInstance()->ParticleDebugGUI();
#endif // _DEBUG
}

void ParticleDebugScene::BlackFade() {
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
	
	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	}

}
