#include "GameScene.h"
#include "Rendering/PrimitiveDrawer.h"

GameScene::GameScene(){
	playerModels_.clear();
}

GameScene::~GameScene(){
	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	delete sphere;
	delete suzunne;
	delete fence;
	===================================================*/
	delete ground;
}

void GameScene::Initialize(){
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト

	sphere = new Model();
	sphere = Model::CreateSphere();

	suzunne = new Model();
	suzunne = suzunne->CreateOBJ("suzanne.obj");

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		Model* newModel = new Model(*suzunne);
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		newModel->SetWVP();
		suzunnes.push_back(newModel);
		addDis += 0.5f;

	}

	fence = new Model();
	fence = fence->CreateOBJ("fence.obj");

	===================================================*/

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        3dモデル                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	ground = new Model();
	ground = ground->CreateOBJ("ground.obj");
	ground->transform.translate = Vector3 {-2.5f,0.0f,0.0f};


	Model* playerModel = new Model();
	playerModel = playerModel->CreateOBJ("debugCube.obj");

	playerModels_.push_back(playerModel);
	player_ = std::make_unique<Player>();
	player_->Initialize(playerModels_);


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        サウンド                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	soundData1 = audio_->SoundLoadWave("resource/Alarm01.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

}

void GameScene::Update(){


#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)){
		if (isDebugCameraMode_){
			isDebugCameraMode_ = false;
			dxCommon_->SetIsDebugCamera(false);
		} else{
			isDebugCameraMode_ = true;
			dxCommon_->SetIsDebugCamera(true);
		}
	}

	if (isDebugCameraMode_){
		DebugCamera::GetInstance()->Update();
	}


#endif // _DEBUG

	if (input_->PushKey(DIK_LEFT)){
		ground->transform.translate.x -= 0.05f;
	}
	if (input_->PushKey(DIK_RIGHT)){
		ground->transform.translate.x += 0.05f;
	}
	if (input_->PushKey(DIK_UP)){
		ground->transform.translate.y += 0.05f;
	}
	if (input_->PushKey(DIK_DOWN)){
		ground->transform.translate.y -= 0.05f;
	}


	if (input_->TriggerKey(DIK_8)){
		audio_->SoundPlayWave(soundData1);
	}
	if (input_->TriggerKey(DIK_9)){
		audio_->SoundStopWave(soundData1);
	}
	if (input_->TriggerKey(DIK_7)){
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

	//プレイヤーの更新
	player_->Update();

}

void GameScene::Draw(){

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

	//プレイヤーの描画
	player_->Draw();


	//描画コマンドを積んでます
	PrimitiveDrawer::GetInstance()->Render();
#pragma endregion


#pragma region 前景スプライト

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}
