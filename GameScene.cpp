#include "GameScene.h"

GameScene::GameScene() {}

GameScene::~GameScene() {
	delete sphere;
	delete suzunne;
}

void GameScene::Initialize() {
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	debugCamera_ = std::make_unique<DebugCamera>();

	sphere = new Model();
	sphere = Model::CreateSphere();

	suzunne = new Model();
	suzunne = suzunne->CreateOBJ("suzanne.obj");

}

void GameScene::Update() {


#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)) {
		if (isDebugCameraMode_) {
			isDebugCameraMode_ = false;
		} else {
			isDebugCameraMode_ = true;
		}
	}

	if (isDebugCameraMode_)
	{
		debugCamera_->Update();
	}


#endif // _DEBUG

	dxCommon_->UpDate();
	sphere->SetWVP();

}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	dxCommon_->PreModelDraw();
	sphere->Draw();

#pragma endregion


#pragma region 前景スプライト

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}
