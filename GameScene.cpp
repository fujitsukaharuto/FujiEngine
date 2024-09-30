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

	if (input_->PushKey(DIK_LEFT)) {
		suzunne->transform.translate.x -= 0.05f;
	}
	if (input_->PushKey(DIK_RIGHT)) {
		suzunne->transform.translate.x += 0.05f;
	}
	if (input_->PushKey(DIK_UP)) {
		suzunne->transform.translate.y += 0.05f;
	}
	if (input_->PushKey(DIK_DOWN)) {
		suzunne->transform.translate.y -= 0.05f;
	}



	dxCommon_->UpDate();
	suzunne->transform.rotate.y = 3.14f;
	suzunne->transform.rotate.x += 0.01f;
	suzunne->SetWVP();
	sphere->transform.rotate.y += 0.01f;
	sphere->SetWVP();

}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	dxCommon_->PreModelDraw();
	sphere->Draw();
	suzunne->Draw();

#pragma endregion


#pragma region 前景スプライト

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}
