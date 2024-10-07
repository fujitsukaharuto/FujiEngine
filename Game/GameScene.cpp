#include "GameScene.h"
#include "ModelManager.h"
#include "GlobalVariables.h"

GameScene::GameScene(){
	playerModels_.clear();
	bossModels_.clear();
}

GameScene::~GameScene(){
	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	delete sphere;
	delete suzunne;
	delete fence;
	===================================================*/
	delete ground;

	player_.reset();
	playerModels_.clear();

	boss_.reset();
	bossModels_.clear();
}

void GameScene::Initialize(){
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	/*GlobalVariables* globalvariables = GlobalVariables::GetInstance();
	const char* groupName = "Sphere";
	const char* groupName2 = "Fence";*/


	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト

	globalvariables->CreateGroup(groupName);
	globalvariables->AddItem(groupName, "parametar", spherePara);
	globalvariables->AddItem(groupName, "Position", spherevec);

	globalvariables->CreateGroup(groupName2);
	globalvariables->AddItem(groupName2, "parametar", fencePara);
	globalvariables->AddItem(groupName2, "Position", fencevec);


	sphere = ModelManager::CreateSphere();

	suzunne = ModelManager::LoadOBJ("suzanne.obj");

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		Model* newModel = ModelManager::LoadOBJ("suzanne.obj");
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		newModel->SetWVP();
		suzunnes.push_back(newModel);
		addDis += 0.5f;

	}

	fence = ModelManager::LoadOBJ("Fence.obj");

	===================================================*/

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        3dモデル                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	ground = ModelManager::LoadOBJ("ground.obj");
	ground->transform.translate = Vector3 {-2.5f,0.0f,0.0f};


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        プレイやー                                            */
	////////////////////////////////////////////////////////////////////////////////////////////////
	Model* playerModel = ModelManager::LoadOBJ("debugCube.obj");
	playerModels_.emplace_back(playerModel);
	player_ = std::make_unique<Player>();
	player_->Initialize(playerModels_);

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                          敵関連                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	Model* bossModel = ModelManager::LoadOBJ("debugCube.obj");
	bossModels_.emplace_back(bossModel);
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(bossModels_);
	boss_->SetTranslate(Vector3 {-4.0f,-1.0f,0.0f});


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        サウンド                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	soundData1 = audio_->SoundLoadWave("resource/Alarm01.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

	//ApplyGlobalVariables();
	
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

	//ApplyGlobalVariables();

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

	float rotaSpeed = 0.1f;
	for (auto suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed;
		suzunneModel->SetWVP();
		rotaSpeed += 0.05f;
	}


	sphere->transform.translate = spherevec;
	sphere->transform.rotate.y += 0.02f;
	sphere->SetWVP();
	fence->transform.translate = fencevec;
	fence->transform.rotate.x = 0.5f;
	fence->SetWVP();
	================================================*/
	ground->SetWVP();

	//プレイヤーの更新
	player_->Update();

	//ボスの更新
	boss_->Update();

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

	//ボスの描画
	boss_->Draw();


	//描画コマンドを積んでます
#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}

void GameScene::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Sphere";
	const char* groupName2 = "Fence";

	spherePara = globalVariables->GetFloatValue(groupName, "parametar");
	spherevec = globalVariables->GetVector3Value(groupName, "Position");

	fencePara = globalVariables->GetFloatValue(groupName2, "parametar");
	fencevec = globalVariables->GetVector3Value(groupName2, "Position");

}
