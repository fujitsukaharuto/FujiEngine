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
	delete selector_;
	delete inputHandler_;
}

void TitleScene::Initialize() {
	Init();

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	inputHandler_ = new StageSceneInputHandler();
	selector_ = new Selector();
	selector_->Initialize();

	inputHandler_->SetSelector(selector_);

}

void TitleScene::Update() {



#ifdef _DEBUG


#endif // _DEBUG


	if (selector_->GetSelectMode() == SelectMode::SELECTOR) {
		this->command_ = inputHandler_->selectorHandleInput(this->selector_);
	} else if (selector_->GetSelectMode() == SelectMode::UNIT) {
		this->command_ = inputHandler_->UnitHandleInput(selector_->GetSelectedUnitAddress());
	}

	if (this->command_) {
		if (selector_->GetSelectMode() == SelectMode::UNIT) {
			this->command_->Exec();
			if (commandHistory_.empty()) {
				commandHistory_.push_back(this->command_);
				commandHistoryItr_ = commandHistory_.end();
			} else if (commandHistoryItr_ == commandHistory_.end()) {
				commandHistory_.push_back(this->command_);
				commandHistoryItr_ = commandHistory_.end();
			} else {
				*commandHistoryItr_ = this->command_;
				commandHistoryItr_++;
			}
			if (typeid(*this->command_) == typeid(UnitMoveEndCommand)) {
				commandHistory_.clear();
				commandHistoryItr_ = commandHistory_.end();
			}
		} else {
			this->command_->Exec();
		}
	}

	if (Input::GetInstance()->PushKey(DIK_LCONTROL) && Input::GetInstance()->TriggerKey(DIK_Z)) {
		if (commandHistory_.empty() || commandHistoryItr_ == commandHistory_.begin()) return;
		if (commandHistoryItr_ == commandHistory_.end()) {
			commandHistoryItr_--;
		} else {
			--commandHistoryItr_;
		}
		(*commandHistoryItr_)->Exec();
	}
	

	selector_->Update();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();

	selector_->Draw();

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
