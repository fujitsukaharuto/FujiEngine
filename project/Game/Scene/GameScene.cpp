#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "FPSKeeper.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"



GameScene::GameScene() {}

GameScene::~GameScene() {
}

void GameScene::Initialize() {
	Init();

	///-------------------------------------------------------------
	///　生成
	///-------------------------------------------------------------- 
	obj3dCommon = std::make_unique<Object3dCommon>();
	player_ = std::make_unique<Player>();
	field_ = std::make_unique<Field>();

	///-------------------------------------------------------------
	///　初期化
	///-------------------------------------------------------------- 
	obj3dCommon->Initialize();
	field_->Initialize();
	player_->Initialize();
}

void GameScene::Update() {

	///-------------------------------------------------------------
	///　更新
	///-------------------------------------------------------------- 
	field_->Update();
	player_->Update();

	ParticleManager::GetInstance()->Update();

}

void GameScene::Draw() {

#pragma region 背景描画

	///-------------------------------------------------------------
	///　背景描画
	///--------------------------------------------------------------

	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();

	///-------------------------------------------------------------
	///　モデル描画
	///-------------------------------------------------------------- 

	field_->Draw();
	player_->Draw();


	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

	///-------------------------------------------------------------
	///　前景スプライト描画
	///-------------------------------------------------------------- 


#pragma endregion

}

