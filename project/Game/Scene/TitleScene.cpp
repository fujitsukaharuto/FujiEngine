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
	Audio::GetInstance()->SoundStopWave(bgm_);
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


	titleAnimation_ = std::make_unique<TitleAnimationManager>();
	titleAnimation_->Init();

	cMane_ = std::make_unique<CollisionManager>();
	
	bgm_ = Audio::GetInstance()->SoundLoadWave("BGM2.wav");
	Audio::GetInstance()->SoundLoop(bgm_, 0.075f);

	//titleAnimation_->SetOnAnimationCompleteCallback([]() {});
	//titleAnimation_->StartAnimation(); // アニメーションの開始

	//ParticleManager::Load(emit1, "groundBreakReverse");
	//ParticleManager::Load(emit2, "groundCrack2");

}

void TitleScene::Update() {
#ifdef _DEBUG
	titleAnimation_->AdjustParamater();
#endif // _DEBUG
	titleAnimation_->Update();
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

	//emit1.DebugGUI();
	//emit2.DebugGUI();

	//emit1.Emit();
	//emit2.Emit();

	//if (Input::GetInstance()->TriggerKey(DIK_5)) {
	//	emit1.Burst();
	//	//emit2.Burst();
	//}


#endif // _DEBUG
	dxCommon_->UpDate();

	BlackFade();


	sphere->transform.rotate.y += 0.02f;

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

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG

#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	titleAnimation_->BackDraw();
	titleAnimation_->Draw();
	/*titlePaneru_->Draw();*/
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
