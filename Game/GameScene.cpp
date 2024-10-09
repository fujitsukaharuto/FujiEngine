#include "GameScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "Rendering/PrimitiveDrawer.h"
#include "Collision/CollisionManager.h"

#include <array>

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
	player_.reset();
	playerModels_.clear();

	boss_.reset();
	bossModels_.clear();

	field_.reset();

	CollisionManager::GetInstance()->Reset();

	/*for (auto suzunneModel : suzunnes){
		delete suzunneModel;
	}
	delete terrain;
	delete test;*/
}
	

void GameScene::Initialize(){
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	camera.reset(new Camera());

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize(camera.get());

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

	sphere = new Object3d();
	sphere->CreateSphere(obj3dCommon.get());

	suzunne = new Object3d();
	suzunne->Create("suzanne.obj", obj3dCommon.get());

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		Object3d* newModel = new Object3d();
		newModel->Create("suzanne.obj",obj3dCommon.get());
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		suzunnes.push_back(newModel);
		addDis += 0.5f;

	}

	fence = new Object3d();
	fence->Create("Fence.obj", obj3dCommon.get());

	terrain = new Object3d();
	terrain->Create("terrain.obj", obj3dCommon.get());


	test = new Sprite();
	test->Load("uvChecker.png");


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                   フィールド (五線譜)                                        */
	////////////////////////////////////////////////////////////////////////////////////////////////
	
	for (Object3d*& fieldModel : fieldModels_){
		fieldModel = new Object3d;
		fieldModel->Create("ground.obj", obj3dCommon.get());
	}


	field_ = std::make_unique<Field>();
	field_->Initialize(fieldModels_);


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        プレイやー                                            */
	////////////////////////////////////////////////////////////////////////////////////////////////
	Object3d* playerModel = new Object3d();
	playerModel->Create("debugCube.obj",obj3dCommon.get());
	playerModels_.emplace_back(playerModel);
	player_ = std::make_unique<Player>();
	player_->Initialize(playerModels_);

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                          敵関連                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////

	//=======================================================================================
	//↓boss
	Object3d* bossModel = new Object3d(); 
	bossModel->Create("debugCube.obj",obj3dCommon.get());
	bossModels_.emplace_back(bossModel);
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(bossModels_);
	boss_->SetTranslate(Vector3 {-4.0f,-1.0f,0.0f});
	
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize();
	enemyManager_->SetField(field_.get());
	enemyManager_->SetObject3dCommon(obj3dCommon.get());

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        サウンド                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////
	soundData1 = audio_->SoundLoadWave("resource/Alarm01.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

	//ApplyGlobalVariables();
	
}

void GameScene::Update(){

	camera->Update();

#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)){
		if (isDebugCameraMode_){
			isDebugCameraMode_ = false;

		} else {
			isDebugCameraMode_ = true;

		}
	}

	if (isDebugCameraMode_){
		DebugCamera::GetInstance()->Update();
	}

	//ApplyGlobalVariables();

	//ImGui::Begin("suzunne");

	//ImGui::ColorEdit4("color", &color_.X);
	////suzunne->SetColor(color_);
	//ImGui::End();

	//ImGui::Begin("Sphere");

	//ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	//ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	//ImGui::DragFloat3("right", &rightDir.x,0.01f);
	//rightDir = rightDir.Normalize();
	//sphere->SetRightDir(rightDir);
	//ImGui::End();


#endif // _DEBUG

	if (input_->PushKey(DIK_LEFT)){
	}
	if (input_->PushKey(DIK_RIGHT)){
	}
	if (input_->PushKey(DIK_UP)){
	}
	if (input_->PushKey(DIK_DOWN)){
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
	suzunne->transform.rotate.x += 0.05f;


	float rotaSpeed = 0.1f;
	for (auto suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed;

		rotaSpeed += 0.05f;
	}


	sphere->transform.translate = spherevec;
	sphere->transform.rotate.y += 0.02f;

	fence->transform.translate = fencevec;
	fence->transform.rotate.x = 0.5f;
	fence->SetWVP();
	================================================*/
	field_->Update();

	//プレイヤーの更新
	player_->Update();

	//ボスの更新
	boss_->Update();


	enemyManager_->Update();

	//衝突判定
	CollisionManager::GetInstance()->CheckAllCollidion();
}

void GameScene::Draw(){

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	/*sphere->Draw();
	suzunne->Draw();
	fence->Draw();*/
	field_->Draw();

	//プレイヤーの描画
	player_->Draw();

	//ボスの描画
	boss_->Draw();

	enemyManager_->Draw();


	//描画コマンドを積んでます
	
	//for (auto suzunneModel : suzunnes) {
	//	suzunneModel->Draw();
	//}
	//terrain->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}

void GameScene::LoadEnemyPopData(){

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
