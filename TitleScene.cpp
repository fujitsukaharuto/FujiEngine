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
	delete space;
	delete title;
}

void TitleScene::Initialize() {


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	sphere = new Object3d();
	sphere->CreateSphere();

	title = new Object3d();
	title->Create("Title.obj");
	title->transform.rotate.y = 3.14f;
	title->transform.translate.y = 4.0f;
	title->transform.scale = { 2.5f,2.5f,2.5f };

	CameraManager::GetInstance()->GetCamera()->transform = { { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } };

	space = new Sprite();
	space->Load("space.png");
	space->SetPos({ 640.0f,550.0f,0.0f });
	space->SetSize({ 400.0f,200.0f });


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


	if (moveTime <= 40.0f) {

		moveTime += FPSKeeper::DeltaTime();

		if (moveTime >= 20.0f && moveTime <= 30.0f) {
			float t = moveTime - 20.0f;
			t = t / 30.0f;
			title->transform.scale.y = Lerp(3.5f, 2.0f, 1.0f - powf(1.0f - t, 3));
		}
		else if (moveTime > 30.0f && moveTime <= 35.0f) {
			float t = moveTime - 30.0f;
			t = t / 35.0f;
			title->transform.scale.y = Lerp(2.0f, 3.5f, powf(t, 4));
		}
	}
	else {
		moveTime = 0.0f;
	}

	if (startTime <= 0.0f) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
		}
	}
	else {
		startTime--;
	}


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

void TitleScene::SpriteDraw() {

	dxCommon_->ClearDepthBuffer();
	obj3dCommon->PreDraw();
	title->Draw();

	dxCommon_->PreSpriteDraw();
	space->Draw();
}

void TitleScene::ApplyGlobalVariables() {


}
