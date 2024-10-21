#include "GameScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "Rendering/PrimitiveDrawer.h"
#include "Collision/CollisionManager.h"
#include "FPSKeeper.h"
#include "Random.h"

#include "SceneManager.h"
#include "ParticleManager.h"
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

void GameScene::Initialize() {


	Vector3 cameraInitializePos = {-11.5f,5.6f,-63.5f};
	Vector3 cameraInitializeRotate = {0.0f, 0.37f, 0.0f};
	CameraManager::GetInstance()->GetCamera()->transform.translate = cameraInitializePos;
	CameraManager::GetInstance()->GetCamera()->transform.rotate = cameraInitializeRotate;


	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	GlobalVariables* globalvariables = GlobalVariables::GetInstance();
	/*const char* groupName = "Sphere";
	const char* groupName2 = "Fence";*/
	const char* groupName = "camera";
	globalvariables->CreateGroup(groupName);
	globalvariables->AddItem(groupName, "cameraSpeed", cameraMoveSpeed_);


	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト

	globalvariables->CreateGroup(groupName);
	globalvariables->AddItem(groupName, "parametar", spherePara);
	globalvariables->AddItem(groupName, "Position", spherevec);

	globalvariables->CreateGroup(groupName2);
	globalvariables->AddItem(groupName2, "parametar", fencePara);
	globalvariables->AddItem(groupName2, "Position", fencevec);

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	sphere = new Object3d();
	sphere->CreateSphere(obj3dCommon.get());

	suzunne = new Object3d();
	suzunne->Create("suzanne.obj", obj3dCommon.get());

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		Object3d* newModel = new Object3d();
		newModel->Create("suzanne.obj");
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		suzunnes.push_back(newModel);
		addDis += 0.5f;

	}

	fence = new Object3d();
	fence->Create("Fence.obj");

	terrain = new Object3d();
	terrain->Create("terrain.obj");


	test = new Sprite();
	test->Load("uvChecker.png");


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                   フィールド (五線譜)                                        */
	////////////////////////////////////////////////////////////////////////////////////////////////

	for (Object3d*& fieldModel : fieldModels_){
		fieldModel = new Object3d;
		fieldModel->Create("ground.obj");
		fieldModel->SetColor({ 0.2f,0.2f,0.2f,1.0f });
	}


	field_ = std::make_unique<Field>();
	field_->Initialize(fieldModels_);


	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        プレイやー                                            */
	////////////////////////////////////////////////////////////////////////////////////////////////
	Object3d* playerModel = new Object3d();
	playerModel->Create("player.obj");
	playerModels_.emplace_back(playerModel);

	float playerInitiOffset = 10.0f;
	Vector3 playerInitPosition = {field_->fieldEndPosX + playerInitiOffset,0.0f,0.0f};
	player_ = std::make_unique<Player>();
	player_->Initialize(playerModels_);
	player_->SetRotate({ 0.0f,3.14f,0.0f });
	player_->SetTranslate(playerInitPosition);

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                          敵関連                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////

	//=======================================================================================
	//↓boss
	Object3d* bossModel = new Object3d();
	bossModel->Create("boss.obj");
	bossModels_.emplace_back(bossModel);

	boss_ = std::make_unique<Boss>();
	boss_->Initialize(bossModels_);
	boss_->SetTranslate(Vector3 {-8.0f,3.0f,-1.8f});//五線譜の真ん中に合わせる
	boss_->SetScale(Vector3 {1.0f,1.0f,1.0f});
	boss_->SetRotate(Vector3{ 0.0f,3.14f,0.0f });

	//ポインタをフィールドに渡す
	field_->SetBoss(boss_.get());
	//=======================================================================================
	//↓音符になる敵(管理クラス)
	ModelManager::GetInstance()->LoadOBJ("enemy.obj");
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize();
	enemyManager_->SetField(field_.get());
	enemyManager_->SetObject3dCommon(obj3dCommon.get());

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*                                        サウンド                                             */
	////////////////////////////////////////////////////////////////////////////////////////////////



	//ApplyGlobalVariables();

	emit.count = 3;
	emit.frequencyTime = 20.0f;
	emit.name = "animetest";
	emit.pos = { 0.0f,2.0f,0.0f };
	emit.animeData.lifeTime = 40.0f;
	emit.RandomSpeed({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
}

void GameScene::Update(){

	// ボスの移動量に応じてカメラを同じだけ移動させる
	CameraManager::GetInstance()->GetCamera()->transform.translate.x += cameraMoveSpeed_ * FPSKeeper::DeltaTime();


#ifdef _DEBUG
	field_->ShowImgui();

	ApplyGlobalVariables();

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

	ImGui::Begin("Scene");
	ImGui::SeparatorText("ChangeScene");
	if (ImGui::Button("ResultScene")) {
		SceneManager::GetInstance()->ChangeScene("RESULT", 40.0f);
	}
	if (ImGui::Button("TitleScene")) {
		SceneManager::GetInstance()->ChangeScene("TITLE", 40.0f);
	}

	ImGui::End();

#endif // _DEBUG

	if (input_->PushKey(DIK_LEFT)){
	}
	if (input_->PushKey(DIK_RIGHT)){
	}
	if (input_->PushKey(DIK_UP)){
	}
	if (input_->PushKey(DIK_DOWN)){
	}


	dxCommon_->UpDate();

	/*===============================================
	ゲーム画面を作成するので一時的にコメントアウト
	suzunne->transform.rotate.y = 3.14f;
	suzunne->transform.rotate.x += (0.05f) * FPSKeeper::DeltaTime();


	float rotaSpeed = 0.1f;
	for (auto suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed * FPSKeeper::DeltaTime();
		//suzunneModel->transform.translate = Random::GetVector3({ -4.0f,4.0f }, { -4.0f,4.0f }, { -4.0f,4.0f });
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

	ParticleManager::GetInstance()->Update();

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




	//for (auto suzunneModel : suzunnes) {
	//	suzunneModel->Draw();
	//}
	//terrain->Draw();

	//描画コマンドを積んでます
	PrimitiveDrawer::GetInstance()->Render();

	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();



#pragma endregion




}

void GameScene::LoadEnemyPopData(){

}

void GameScene::ApplyGlobalVariables(){
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	/*const char* groupName = "Sphere";
	const char* groupName2 = "Fence";

	spherePara = globalVariables->GetFloatValue(groupName, "parametar");
	spherevec = globalVariables->GetVector3Value(groupName, "Position");

	fencePara = globalVariables->GetFloatValue(groupName2, "parametar");
	fencevec = globalVariables->GetVector3Value(groupName2, "Position");*/

	const char* groupName = "camera";
	cameraMoveSpeed_ = globalVariables->GetFloatValue(groupName, "cameraSpeed");


}
