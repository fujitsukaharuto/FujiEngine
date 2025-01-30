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


#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,950.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	titlePaneru_ = std::make_unique<Sprite>();
	titlePaneru_->Load("title.png");
	titlePaneru_->SetAnchor({ 0.0f,0.0f });


	sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	test_ = std::make_unique<TestBaseObj>();
	test_->Initialize();
	test_->name_ = "testObj";
	test_->GetCollider()->SetIsCollisonCheck(false);

	test2_ = std::make_unique<TestBaseObj>();
	test2_->Initialize();
	test2_->name_ = "testObj2";

	cMane_ = std::make_unique<CollisionManager>();

	/*ParticleManager::Load(emit1, "lifeUIBreak1");*/
	/*ParticleManager::Load(emit2, "lifeUIBreak2");*/

}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG

	ImGuiManager::GetInstance()->SetFontJapanese();
	ImGui::Begin("円の調整");
	ImGuiManager::GetInstance()->UnSetFont();
	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();

	/*emit1.DebugGUI();*/
	/*emit2.DebugGUI();*/

	test_->Debug();
	test2_->Debug();

#endif // _DEBUG


	dxCommon_->UpDate();

	BlackFade();

	test_->Update();
	test2_->Update();


	//if (input_->TriggerKey(DIK_5)) {
	//	emit1.Burst();
	//	/*emit2.Burst();*/
	//}
	/*emit1.Emit();*/
	/*emit2.Emit();*/

	sphere->transform.rotate.y += 0.02f;

	cMane_->AddCollider(test_->GetCollider());
	cMane_->AddCollider(test2_->GetCollider());
	cMane_->CheckAllCollision();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	sphere->Draw();
	test_->Draw();

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	test_->DrawCollider();
	test2_->DrawCollider();
#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

	titlePaneru_->Draw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion

}

void TitleScene::BlackFade() {
	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		}
		else {
			SceneManager::GetInstance()->ChangeScene("GAME", 40.0f);
		}
	}
	else {
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
	}

	else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
}

void TitleScene::ApplyGlobalVariables() {


}
