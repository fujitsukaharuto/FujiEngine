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
#include "Math/MatrixCalculation.h"

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

	sphere1 = std::make_unique<Object3d>();
	sphere1->CreateSphere();

	sphere2 = std::make_unique<Object3d>();
	sphere2->CreateSphere();
	sphere2->transform.translate.z = 30.0f;

	cMane_ = std::make_unique<CollisionManager>();


}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG


	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere1->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere1->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x, 0.01f);
	rightDir = rightDir.Normalize();
	sphere1->SetRightDir(rightDir);
	sphere2->SetRightDir(rightDir);
	ImGui::End();


#endif // _DEBUG


	dxCommon_->UpDate();

	BlackFade();

	
	SphereUpdate();


	cMane_->CheckAllCollision();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	sphere1->Draw();
	sphere2->Draw();
	

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
	/*XINPUT_STATE pad;
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
	}*/
}

void TitleScene::SphereUpdate() {

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		velocity1_ = { 0.0f,0.0f,0.5f };
	}
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		velocity2_ = { 0.25f,0.0f,0.0f };
	}
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		velocity1_ = { 0.0f,0.0f,0.0f };
		velocity2_ = { 0.25f,0.0f,0.0f };
		sphere1->transform.translate = { 0.0f,0.0f,0.0f };
		sphere2->transform.translate = { 0.0f,0.0f,20.0f };
	}

	if (sphere2->transform.translate.x < -20.0f || sphere2->transform.translate.x > 20.0f) {
		velocity2_.x = -velocity2_.x;
	}

	Vector3 nextPos1 = sphere1->transform.translate + velocity1_ * FPSKeeper::DeltaTime();
	Vector3 nextPos2 = sphere2->transform.translate + velocity2_ * FPSKeeper::DeltaTime();


	Vector3 veloNormal = nextPos2 - nextPos1;
	float dist = veloNormal.Length();

	if (dist <= 2.0f) {

		if (dist <= 0.000001f) {
			veloNormal = Vector3(0.000001f, 0, 0);
		}
		else {
			veloNormal = veloNormal.Normalize();
		}

		float overlap = (1.0f + 1.0f) - dist;
		if (overlap > 0) {
			sphere1->transform.translate += -veloNormal * (overlap * 0.5f);
			sphere2->transform.translate += veloNormal * (overlap * 0.5f);
		}

		std::pair<Vector3, Vector3> velos = ComputeCollisionVelocity(mass1, velocity1_, mass2, velocity2_, restituion, veloNormal);
		velocity1_ = velos.first;
		velocity2_ = velos.second;

		nextPos1 = sphere1->transform.translate + velocity1_ * FPSKeeper::DeltaTime();
		nextPos2 = sphere2->transform.translate + velocity2_ * FPSKeeper::DeltaTime();
	}

	sphere1->transform.translate = nextPos1;
	sphere2->transform.translate = nextPos2;
}

std::pair<Vector3, Vector3> TitleScene::ComputeCollisionVelocity(float m1, const Vector3& v1, float m2, const Vector3& v2, float ref, const Vector3& normal) {

	Vector3 normalN = normal.Normalize();

	Vector3 vRel = v1 - v2;
	Vector3 vRelN = Vector3::Project(vRel, normalN);
	Vector3 vRelN_after = -ref * vRelN;

	Vector3 v1_after = v1 - (m2 / (m1 + m2)) * (vRelN - vRelN_after);
	Vector3 v2_after = v2 + (m1 / (m1 + m2)) * (vRelN - vRelN_after);

	return std::make_pair(v1_after, v2_after);
}

void TitleScene::ApplyGlobalVariables() {


}
