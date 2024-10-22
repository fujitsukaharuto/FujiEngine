#include "ResultScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"

#include "SceneManager.h"
#include "ParticleManager.h"



ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	delete sphere;
	delete text;
}

void ResultScene::Initialize() {


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	sphere = new Object3d();
	sphere->CreateSphere();
	sphere->SetEnableLight(LightMode::kPointLightON);
	sphere->SetColor({ 1.0f,0.0f,0.0f,1.0f });


	if (SceneManager::GetInstance()->GetClear()) {
		text = new Object3d();
		text->Create("clear.obj");
	}
	if (SceneManager::GetInstance()->GetGameover()) {
		text = new Object3d();
		text->Create("gameOver.obj");
	}
	text->transform.translate.y = 2.0f;
	text->transform.rotate.y = -3.14f;
	text->transform.scale = { 2.0f,2.0f,2.0f };
	text->UpdateWorldMat();


	CameraManager::GetInstance()->GetCamera()->transform = { { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } };

}

void ResultScene::Update() {

#ifdef _DEBUG


	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	ImGui::DragFloat("ins", &ins, 0.1f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();

	sphere->SetRightIntensity(ins);

	ImGui::Begin("Scene");
	ImGui::SeparatorText("ChangeScene");
	if (ImGui::Button("GameScene")) {
		SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
	}
	if (ImGui::Button("TitleScene")) {
		SceneManager::GetInstance()->ChangeScene("TITLE", 40.0f);
	}
	ImGui::End();


#endif // _DEBUG


	dxCommon_->UpDate();

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChangeScene("TITLE", 40.0f);
	}

	sphere->transform.rotate.y += 0.02f;



	ParticleManager::GetInstance()->Update();
}

void ResultScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	sphere->Draw();

	text->Draw();

	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion




}

void ResultScene::ApplyGlobalVariables() {


}
