#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Random.h"
#include "Model/Line3dDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
	delete inputHandler_;
	if (iCommand_) {
		delete iCommand_;
	}
	delete player_;
}

void TitleScene::Initialize() {
	Init();

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();


	inputHandler_ = new InputHandler();

	inputHandler_->AssignMoveRightCommand2PressKeyD();
	inputHandler_->AssignMoveLeftCommand2PressKeyA();

	player_ = new Player();
	player_->Initialize();

}

void TitleScene::Update() {



#ifdef _DEBUG


#endif // _DEBUG


	iCommand_ = inputHandler_->HandleInput();

	if (iCommand_) {
		iCommand_->Exec(*player_);
	}

	player_->Update();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();

	player_->Draw();

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG


#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

#pragma endregion

}

void TitleScene::ApplyGlobalVariables() {


}
