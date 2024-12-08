#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"
#include "Model/Line3dDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
}

void TitleScene::Initialize() {
	Init();

	

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	/*sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();*/

	emit.name = "sphere";
	emit.Load("sphere");

	kira.name = "kira";
	kira.Load("kira");

}

void TitleScene::Update() {

#ifdef _DEBUG


	/*ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();*/

	emit.DebugGUI();
	kira.DebugGUI();

#endif // _DEBUG


	dxCommon_->UpDate();


	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		dxCommon_->SetThunder(false);
		dxCommon_->SetFire(true);
		SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
	}

	if (input_->TriggerKey(DIK_5)) {
		emit.Emit();
	}
	emit.Emit();
	kira.Emit();

	//sphere->transform.rotate.y += 0.02f;



	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	/*sphere->Draw();*/


	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	emit.DrawSize();
	kira.DrawSize();
#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion

}

void TitleScene::ApplyGlobalVariables() {


}
