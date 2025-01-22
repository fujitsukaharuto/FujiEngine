#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"
#include "Model/Line3dDrawer.h"
#include "Model/PlaneDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
}

void TitleScene::Initialize() {
	Init();

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });

	paneru = std::make_unique<Sprite>();
	paneru->Load("title2.png");
	paneru->SetSize({ 1280.0f,720.0f });
	paneru->SetAnchor({ 0.0f,0.0f });

	/*sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	test_ = std::make_unique<TestBaseObj>();
	test_->Initialize();
	test_->name_ = "testObj";

	test2_ = std::make_unique<TestBaseObj>();
	test2_->Initialize();
	test2_->name_ = "testObj2";*/

	/*emit.name = "sphere";
	emit.Load("sphere");*/

	emit1.name = "bomb1";
	emit1.Load("bomb1");
	emit2.name = "bomb2";
	emit2.Load("bomb2");
	emit3.name = "bomb3";
	emit3.Load("bomb3");

}

void TitleScene::Update() {

#ifdef _DEBUG



#endif // _DEBUG


	dxCommon_->UpDate();

	BlackFade();

	/*test_->Update();
	test2_->Update();*/

	emit1.Emit();
	emit2.Emit();
	emit3.Emit();

#ifdef _DEBUG
	emit1.DebugGUI();
	emit2.DebugGUI();
	emit3.DebugGUI();
#endif // _DEBUG


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

//#ifdef _DEBUG
//	emit1.DrawSize();
//	emit2.DrawSize();
//	emit3.DrawSize();
//#endif // _DEBUG
	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	/*emit.DrawSize();
	test_->DrawCollider();
	test2_->DrawCollider();*/

#endif // _DEBUG
	PlaneDrawer::GetInstance()->Render();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	//paneru->Draw();
	black_->Draw();

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
	if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
	else {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
}

void TitleScene::ApplyGlobalVariables() {


}
