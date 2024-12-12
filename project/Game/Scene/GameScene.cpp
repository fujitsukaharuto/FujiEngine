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

	
}

void GameScene::Update() {

	ParticleManager::GetInstance()->Update();

}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();


	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();


#pragma endregion

}

void GameScene::ApplyGlobalVariables() {

}
