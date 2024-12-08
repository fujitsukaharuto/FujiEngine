#include "ResultScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"
#include "Model/Line3dDrawer.h"


ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
}

void ResultScene::Initialize() {
	Init();


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	/*sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();
	sphere->SetColor({ 1.0f,0.0f,0.0f,1.0f });*/

	emit.count = 10;
	emit.frequencyTime = 13.0f;
	emit.name = "sphere";
	emit.pos = { 0.0f,3.0f,-1.0f };
	emit.grain.lifeTime_ = 13.0f;
	emit.grain.startSize = { 0.75f,0.75f };
	emit.grain.endSize = { 0.01f,0.01f };
	emit.grain.type = SizeType::kShift;
	emit.para_.colorMax = { 1.0f,0.0f,0.0f,0.6f };
	emit.RandomSpeed({ -0.25f,0.25f }, { -0.25f,0.25f }, { -0.1f,0.1f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });


	kira.count = 1;
	kira.frequencyTime = 13.0f;
	kira.name = "kira";
	kira.pos = { 0.0f,3.0f,0.0f };
	kira.emitSizeMax = { 0.0f,0.0f,0.0f };
	kira.emitSizeMin = { 0.0f,0.0f,0.0f };
	kira.grain.lifeTime_ = 13.0f;
	kira.particleRotate.z = 1.0f;
	kira.para_.colorMax = { 1.0f,1.0f,0.0f,0.5f };
	kira.grain.startSize = { 0.65f,2.5f };
	kira.grain.endSize = { 13.0f,0.1f };
	kira.grain.type = SizeType::kShift;
	kira.RandomSpeed({ 0.0f,0.0f }, { 0.0f,0.0f }, { 0.0f,0.0f });
	kira.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });

}

void ResultScene::Update() {

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
		dxCommon_->SetNone(false);
		dxCommon_->SetThunder(true);
		SceneManager::GetInstance()->ChangeScene("TITLE", 40.0f);
	}

	/*sphere->transform.rotate.y += 0.02f;*/

	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		emit.Burst();
		kira.Burst();
	}

	ParticleManager::GetInstance()->Update();
}

void ResultScene::Draw() {

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

void ResultScene::ApplyGlobalVariables() {


}
