#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"

#include "ParticleManager.h"
#include "Scene/SceneManager.h"


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
	delete nightSky;

}

void TitleScene::Initialize() {
	Init();


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();


	nightSky = new Object3d();
	nightSky->Create("nightSky.obj");
	nightSky->transform.scale = { 6.0f,6.0f,6.0f };



}

void TitleScene::Update() {

#ifdef _DEBUG




#endif // _DEBUG


	dxCommon_->UpDate();


	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChangeScene("GAME", 30.0f);
	}


	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

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

#pragma endregion

	


}

void TitleScene::ApplyGlobalVariables() {


}
