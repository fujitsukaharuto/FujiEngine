#include "ResultScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Math/Random/Random.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"



ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
}

void ResultScene::Initialize() {

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	CameraManager::GetInstance()->GetCamera()->transform.rotate = { -1.02f,0.0f,0.0f };
	CameraManager::GetInstance()->GetCamera()->transform.translate = { 0.0f, 3.5f, -20.0f };

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

	sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();
	sphere->SetColor({ 1.0f,0.0f,0.0f,1.0f });

}

void ResultScene::Update() {

#ifdef _DEBUG


#endif // _DEBUG

	BlackFade();

	sphere->transform.rotate.y += 0.02f;



	ParticleManager::GetInstance()->Update();
}

void ResultScene::Draw() {

#pragma region 背景描画


	dxcommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	sphere->Draw();

	clear_->Draw();

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

	if (ImGui::CollapsingHeader("Sphere")) {
		ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
		ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	}

	ImGui::Unindent();
#endif // _DEBUG
}

void ResultScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

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
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	} else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
}

void ResultScene::ApplyGlobalVariables() {


}
