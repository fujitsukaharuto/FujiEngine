#include "ResultScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Particle/Random.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"



ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	delete nightSky;
}

void ResultScene::Initialize() {
	Init();


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	nightSky = new Object3d();
	nightSky->Create("nightSky.obj");
	nightSky->transform.scale = { 6.0f,6.0f,6.0f };


	result.reset(new Sprite);
	result->Load("Sprite/Clear.png");
	result->SetSize({ 600.0f,400.0f });
	result->SetPos({ 640.0f,360.0f,0.0f });

}

void ResultScene::Update() {

#ifdef _DEBUG





#endif // _DEBUG


	dxCommon_->UpDate();

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChangeScene("TITLE", 30.0f);
	}




	ParticleManager::GetInstance()->Update();
}

void ResultScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	nightSky->Draw();


	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	result->Draw();

#pragma endregion




}

void ResultScene::ApplyGlobalVariables() {


}
