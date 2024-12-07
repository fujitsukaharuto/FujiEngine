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

	sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	emit.count = 5;
	emit.frequencyTime = 13.0f;
	emit.name = "sphere";
	emit.pos = { 0.0f,0.4f,0.0f };
	emit.grain.lifeTime_ = 50.0f;
	emit.grain.startSize = { 0.05f,0.05f };
	emit.grain.endSize = { 0.03f,0.03f };
	emit.grain.type = SizeType::kShift;
	//emit.animeData.lifeTime = 40.0f;
	emit.RandomSpeed({ -0.1f,0.1f }, { 0.0f,0.1f }, { -0.1f,0.1f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });


	kira.count = 3;
	kira.frequencyTime = 13.0f;
	kira.name = "kira";
	kira.pos = { 0.0f,0.4f,0.0f };
	kira.emitSizeMax = { 3.0f,2.0f,-1.0f };
	kira.emitSizeMin = { -3.0f,0.0f,1.0f };
	kira.grain.lifeTime_ = 30.0f;
	kira.grain.startSize = { 0.03f,0.03f };
	kira.grain.endSize = { 0.1f,0.1f };
	kira.grain.type = SizeType::kSin;
	kira.RandomSpeed({ -0.0f,0.0f }, { 0.0f,0.0f }, { -0.0f,0.0f });
	kira.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });

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

	emit.DebugGUI();
	kira.DebugGUI();

#endif // _DEBUG


	dxCommon_->UpDate();


	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
	}

	if (input_->TriggerKey(DIK_5)) {
		emit.Emit();
	}
	emit.Emit();
	kira.Emit();

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
