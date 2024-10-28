#include "ResultScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"

#include "Particle/ParticleManager.h"



ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	delete sphere;
}

void ResultScene::Initialize() {
	Init();


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	sphere = new Object3d();
	sphere->CreateSphere();
	sphere->SetColor({ 1.0f,0.0f,0.0f,1.0f });

}

void ResultScene::Update() {

#ifdef _DEBUG


	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();


#endif // _DEBUG


	dxCommon_->UpDate();


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


	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}

void ResultScene::ApplyGlobalVariables() {


}
