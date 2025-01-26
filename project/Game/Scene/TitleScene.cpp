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
#include "Light/PointLightManager.h"


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
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	terain = std::make_unique<Object3d>();
	terain->Create("terrain.obj");

	plane = std::make_unique<Object3d>();
	plane->Create("plane.gltf");

	plane2 = std::make_unique<Object3d>();
	plane2->Create("plane.obj");
	plane2->transform.translate.y = 3.0f;

	/*test_ = std::make_unique<TestBaseObj>();
	test_->Initialize();
	test_->name_ = "testObj";
	test_->GetCollider()->SetIsCollisonCheck(false);

	test2_ = std::make_unique<TestBaseObj>();
	test2_->Initialize();
	test2_->name_ = "testObj2";*/

	cMane_ = std::make_unique<CollisionManager>();

	ParticleManager::Load(emit, "sphere");

}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG


	ImGui::Begin("Sphere");

	ImGui::DragFloat3("trans", &sphere->transform.translate.x, 0.01f);
	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);

	ImGui::End();

	ImGui::Begin("planeGLTF");

	ImGui::DragFloat3("trans", &plane->transform.translate.x, 0.01f);
	ImGui::DragFloat3("scale", &plane->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &plane->transform.rotate.x, 0.01f);

	ImGui::End();

	ImGui::Begin("plane");

	ImGui::DragFloat3("trans", &plane2->transform.translate.x, 0.01f);
	ImGui::DragFloat3("scale", &plane2->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &plane2->transform.rotate.x, 0.01f);

	ImGui::End();

	ImGui::Begin("lightMode");
	ImGui::Combo("lightMode", &mode_, "kLightNone\0kLightHalfLambert\0kLightLambert\0kPhongReflect\0kBlinnPhongReflection\0kPointLightON\0kSpotLightON\0kRectLightON\0");
	sphere->SetLightEnable(static_cast<LightMode>(mode_));
	terain->SetLightEnable(static_cast<LightMode>(mode_));
	plane->SetLightEnable(static_cast<LightMode>(mode_));
	ImGui::End();

	PointLightManager::GetInstance()->GetDirectionLight()->Debug();
	PointLightManager::GetInstance()->GetPointLight(0)->Debug();
	PointLightManager::GetInstance()->GetSpotLight(0)->Debug();
	PointLightManager::GetInstance()->GetRectLight()->Debug();


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
	sphere->Draw();
	plane->DrawGltf();
	plane2->Draw();
	terain->Draw();
	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG

#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
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
