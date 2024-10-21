#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"

#include "SceneManager.h"
#include "ParticleManager.h"



TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
	delete sphere;
}

void TitleScene::Initialize() {
	Init();


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	sphere = new Object3d();
	sphere->CreateSphere();

}

void TitleScene::Update() {

#ifdef _DEBUG


	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();

	ImGui::Begin("Scene");
	ImGui::SeparatorText("ChangeScene");
	if (ImGui::Button("GameScene")) {
		SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
	}
	ImGui::End();

#endif // _DEBUG

	dxCommon_->UpDate();


	sphere->transform.rotate.y += 0.02f;



	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	sphere->Draw();
	

	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion




}

void TitleScene::ApplyGlobalVariables() {


}
