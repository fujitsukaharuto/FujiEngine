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
	delete text;
	for (int i = 0; i < 6; i++) {
		delete chorus[i];
	}
	delete boss;
}

void ResultScene::Initialize() {


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();



	if (SceneManager::GetInstance()->GetClear()) {
		text = new Object3d();
		text->Create("clear.obj");
		defoSize = 2.0f;
		text->transform.scale = { defoSize,defoSize,defoSize };



		boss = new Object3d();
		boss->Create("boss_damage.obj");
		boss->SetCameraParent(true);
		boss->transform.translate.x = 3.0f;
		boss->transform.translate.y = -5.0f;
		boss->transform.translate.z = 60.0f;
		boss->transform.rotate.y = 3.74f;

	}
	if (SceneManager::GetInstance()->GetGameover()) {
		text = new Object3d();
		text->Create("gameOver.obj");



		boss = new Object3d();
		boss->Create("boss.obj");
		boss->SetCameraParent(true);
		boss->transform.translate.x = 3.0f;
		boss->transform.translate.y = -5.0f;
		boss->transform.translate.z = 60.0f;
		boss->transform.rotate.y = 3.74f;

	}
	text->SetCameraParent(true);
	text->transform.translate.y = 1.0f;
	text->transform.translate.z = 20.0f;
	text->transform.rotate.y = -3.14f;
	text->UpdateWorldMat();


	for (int i = 0; i < 3; i++) {
		Object3d* newCho = new Object3d();
		newCho->Create("chorus.obj");
		newCho->SetCameraParent(true);
		newCho->transform.translate.x = 2.0f + 2.25f * i;
		newCho->transform.translate.y = -6.3f;
		newCho->transform.translate.z = 25.0f - i * 1.0f;
		newCho->transform.rotate.y = 3.14f;
		chorus.push_back(newCho);
	}
	for (int i = 0; i < 3; i++) {
		Object3d* newCho = new Object3d();
		newCho->Create("chorus.obj");
		newCho->SetCameraParent(true);
		newCho->transform.translate.x = -2.0f + -2.25f * i;
		newCho->transform.translate.y = -6.3f;
		newCho->transform.translate.z = 25.0f - i * 1.0f;
		newCho->transform.rotate.y = 3.14f;
		chorus.push_back(newCho);
	}


	CameraManager::GetInstance()->GetCamera()->transform = { { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } };

}

void ResultScene::Update() {

#ifdef _DEBUG


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

	if (startTime <= 0.0f) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			SceneManager::GetInstance()->ChangeScene("TITLE", 40.0f);
		}
	}
	else {
		startTime--;
	}




	for (int i = 0; i < 6; i++) {

		// コーラスの移動とアニメーションの処理
		if (chorus[i]->transform.translate.y > -6.45f) {
			chorusTime += FPSKeeper::DeltaTime();

			float sinVal = 0.0f;
			if (i >= 3) {
				sinVal = sin(frequency * chorusTime);
				float angle = amplitude * sinVal;
				chorus[i]->transform.rotate.z = angle;
			}
			else {
				sinVal = sin(frequency * -chorusTime);
				float angle = amplitude * sinVal;
				chorus[i]->transform.rotate.z = angle;
			}

			float scaleMin = 0.7f;  // 最小サイズ
			float scaleMax = 1.2f;  // 最大サイズ
			float scaleRange = scaleMax - scaleMin;

			float scale = scaleMin + (scaleRange * fabs(sinVal));
			chorus[i]->transform.scale.y = scale;
		}

	}




	if (moveTime <= 80.0f) {

		moveTime += FPSKeeper::DeltaTime();

		if (moveTime >= 60.0f && moveTime <= 70.0f) {
			text->transform.scale.y = Lerp(text->transform.scale.y, defoSize - 0.5f, 0.3f);
		}
		else if (moveTime > 70.0f && moveTime <= 80.0f) {
			text->transform.scale.y = Lerp(text->transform.scale.y, defoSize, 0.25f);
		}
	}
	else {
		moveTime = 0.0f;
	}


	ParticleManager::GetInstance()->Update();
}

void ResultScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();


	for (int i = 0; i < 6; i++) {
		chorus[i]->Draw();
	}
	boss->Draw();
	text->Draw();

	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion




}

void ResultScene::ApplyGlobalVariables() {


}
