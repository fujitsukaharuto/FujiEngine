#include "GameScene.h"
#include "Rendering/PrimitiveDrawer.h"
GameScene::GameScene() {}

GameScene::~GameScene() {
	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	delete sphere;
	delete suzunne;
	delete fence;
	===================================================*/
	delete ground;
	playerModels_.clear();
}

void GameScene::Initialize() {
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	
	sphere = new Model();
	sphere = Model::CreateSphere();

	suzunne = new Model();
	suzunne = suzunne->CreateOBJ("suzanne.obj");

	fence = new Model();
	fence = fence->CreateOBJ("fence.obj");

	===================================================*/

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        3dモデル                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	ground = new Model();
	ground = ground->CreateOBJ("ground.obj");
	ground->transform.translate = Vector3{-2.5f,0.0f,0.0f};

	playerModels_.resize(1);
	playerModels_[0] = playerModels_[0]->CreateOBJ("cube.obj");
	player_->Initialize(playerModels_);


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        サウンド                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	soundData1 = audio_->SoundLoadWave("resource/Alarm01.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

}

void GameScene::Update() {


#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)) {
		if (isDebugCameraMode_) {
			isDebugCameraMode_ = false;
			dxCommon_->SetIsDebugCamera(false);
		} else {
			isDebugCameraMode_ = true;
			dxCommon_->SetIsDebugCamera(true);
		}
	}

	if (isDebugCameraMode_)
	{
		DebugCamera::GetInstance()->Update();
	}


#endif // _DEBUG

	if (input_->PushKey(DIK_LEFT)) {
		ground->transform.translate.x -= 0.05f;
	}
	if (input_->PushKey(DIK_RIGHT)) {
		ground->transform.translate.x += 0.05f;
	}
	if (input_->PushKey(DIK_UP)) {
		ground->transform.translate.y += 0.05f;
	}
	if (input_->PushKey(DIK_DOWN)) {
		ground->transform.translate.y -= 0.05f;
	}


	if (input_->TriggerKey(DIK_8)) {
		audio_->SoundPlayWave(soundData1);
	}
	if (input_->TriggerKey(DIK_9)) {
		audio_->SoundStopWave(soundData1);
	}
	if (input_->TriggerKey(DIK_7)) {
		audio_->SoundPlayWave(soundData2);
	}

	dxCommon_->UpDate();
	
	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	suzunne->transform.rotate.y = 3.14f;
	suzunne->transform.rotate.x += 0.2f;
	suzunne->SetWVP();
	sphere->transform.rotate.y += 0.02f;
	sphere->SetWVP();
	fence->transform.translate.x = -3.0f;
	fence->transform.rotate.x = 0.5f;
	fence->SetWVP();
	================================================*/
	ground->SetWVP();

}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	dxCommon_->PreModelDraw();

	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	sphere->Draw();
	suzunne->Draw();
	fence->Draw();
	================================================*/
	ground->Draw();


	//描画コマンドを積んでます
	PrimitiveDrawer::GetInstance()->Render();
#pragma endregion


#pragma region 前景スプライト

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}
