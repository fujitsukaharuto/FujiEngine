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
	sphere->CreateSphere();

	test_ = std::make_unique<TestBaseObj>();
	test_->Initialize();
	test_->name_ = "testObj";

	test2_ = std::make_unique<TestBaseObj>();
	test2_->Initialize();
	test2_->name_ = "testObj2";*/

	cMane_ = std::make_unique<CollisionManager>();

	terrain = std::make_unique<Object3d>();
	terrain->Create("terrain.obj");
	terrain->transform.scale = { 8.0f,8.0f,8.0f };

	player_ = std::make_unique<Player>();
	player_->Initialize();

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());

	/*emit.name = "sphere";
	emit.Load("sphere");*/

}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG


	/*ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();

	emit.DebugGUI();

	test_->Debug();
	test2_->Debug();*/

#endif // _DEBUG


	dxCommon_->UpDate();

	/*test_->Update();
	test2_->Update();*/

	player_->Update();

	followCamera_->Update();

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
	}

	/*if (input_->TriggerKey(DIK_5)) {
		emit.Emit();
	}
	emit.Emit();

	sphere->transform.rotate.y += 0.02f;

	cMane_->AddCollider(test_->GetCollider());
	cMane_->AddCollider(test2_->GetCollider());*/
	cMane_->CheckAllCollision();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	/*sphere->Draw();
	test_->Draw();*/
	terrain->Draw();
	player_->Draw();

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	/*emit.DrawSize();
	test_->DrawCollider();
	test2_->DrawCollider();*/
	player_->DrawCollider();
#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion

}

void TitleScene::ApplyGlobalVariables() {


}
