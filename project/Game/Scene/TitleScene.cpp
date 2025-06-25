#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Math/Random/Random.h"
#include "Model/Line3dDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"
#include "Engine/Editor/CommandManager.h"


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
}

void TitleScene::Initialize() {

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();


#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();

	terrain_ = std::make_unique<Object3d>();
	terrain_->Create("terrain.obj");
	terrain_->LoadTransformFromJson("default_terrainTrnasform.json");
	terrain_->SetUVScale({ 20.0f,20.0f }, { 0.0f,0.0f });

	sphere = std::make_unique<Object3d>();
	sphere->Create("Fence.obj");
	sphere->transform.translate.x = 4.0f;

	sphere2 = std::make_unique<Object3d>();
	sphere2->CreateSphere();
	sphere2->transform.translate.x = 1.0f;

	cube_ = std::make_unique<AnimationModel>();
	cube_->Create("T_boss.gltf");
	cube_->LoadAnimationFile("T_boss.gltf");
	cube_->transform.translate.y = 3.0f;

	test_ = std::make_unique<TestBaseObj>();
	test_->Initialize();
	test_->name_ = "testObj";
	test_->GetCollider()->SetIsCollisonCheck(false);

	test2_ = std::make_unique<TestBaseObj>();
	test2_->Initialize();
	test2_->name_ = "testObj2";

	cMane_ = std::make_unique<CollisionManager>();

	ParticleManager::Load(emit, "sphere");

}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG



#endif // _DEBUG

	BlackFade();
	skybox_->Update();

	test_->Update();
	test2_->Update();

	cube_->AnimationUpdate();

	if (input_->TriggerKey(DIK_5)) {
		emit.Emit();
	}
	emit.Emit();

	cMane_->AddCollider(test_->GetCollider());
	cMane_->AddCollider(test2_->GetCollider());
	cMane_->CheckAllCollision();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

	cube_->CSDispatch();

#pragma region 背景描画


	dxcommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	skybox_->Draw();

	obj3dCommon->PreDraw();
	terrain_->Draw();

	cube_->Draw();


	sphere->Draw();
	sphere2->Draw();
	//test_->Draw();

#ifdef _DEBUG
	CommandManager::GetInstance()->Draw();
#endif // _DEBUG

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	emit.DrawSize();
	test_->DrawCollider();
	test2_->DrawCollider();
#endif // _DEBUG
	cube_->SkeletonDraw();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion

	//test
#pragma region 前景スプライト

	dxcommon_->PreSpriteDraw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion

}

void TitleScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	if (ImGui::CollapsingHeader("Sphere")) {
		sphere->DebugGUI();
	}
	if (ImGui::CollapsingHeader("Sphere2")) {
		sphere2->DebugGUI();
	}
	if (ImGui::CollapsingHeader("cube")) {
		cube_->DebugGUI();
	}
	test_->DebugGUI();
	test2_->DebugGUI();

	ImGui::Unindent();
#endif // _DEBUG
}

void TitleScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	emit.DebugGUI();
	ImGui::Unindent();
#endif // _DEBUG
}

void TitleScene::BlackFade() {
	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		} else {
			if (!isParticleDebugScene_) {
				ChangeScene("GAME", 40.0f);
			} else {
				ChangeScene("PARTICLEDEBUG", 40.0f);
			}
		}
	} else {
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
	} else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
#ifdef _DEBUG
	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
			isParticleDebugScene_ = true;
		}
	}
#endif // _DEBUG
}

void TitleScene::ApplyGlobalVariables() {


}
