#include "SceneManager.h"
#include <cassert>
#include "Engine/DX/DXCom.h"
#include "FPSKeeper.h"
#include "Engine/Scene/BaseScene.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Scene/GameScene.h"
#include "Game/Scene/ResultScene.h"
#include "ImGuiManager.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Particle/ParticleManager.h"


SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

void SceneManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void SceneManager::Finalize() {
	delete scene_;
}

void SceneManager::Update() {
	if (!isChange_) {
		scene_->Update();
		if (isFinifh_) {
			finishTime -= FPSKeeper::DeltaTime();
			if (finishTime <= 0.0f) {

			}
		}
	} else {
		changeExtraTime -= FPSKeeper::DeltaTime();
		if (changeExtraTime <= 0.0f) {
			SceneSet();

		}
	}
}

void SceneManager::Draw() {
	scene_->Draw();
}

void SceneManager::StartScene(const std::string& sceneName) {
	assert(sceneFactory_);

	scene_ = sceneFactory_->CreateScene(sceneName);
	scene_->Init(dxcommon_, this);
	scene_->Initialize();
}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	isChange_ = true;
	changeExtraTime = extraTime;
	finishTime = extraTime * 5.0f;
	if (finishTime == 0.0f) {
		finishTime = 60.0f;
	}

	isFinifh_ = true;;

	nextScene_ = sceneFactory_->CreateScene(sceneName);
}

void SceneManager::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Scene")) {
		scene_->DebugGUI();
		ImGui::SeparatorText("Particle");
		scene_->ParticleDebugGUI();
	}
#endif // _DEBUG
}

void SceneManager::ParticleGroupDebugGUI() {
#ifdef _DEBUG
	scene_->ParticleGroupDebugGUI();
#endif // _DEBUG

}

void SceneManager::SceneSet() {
	if (nextScene_) {
		if (scene_) {
			ParticleManager::ParentReset();
			delete scene_;
		}

		scene_ = nextScene_;
		nextScene_ = nullptr;

		scene_->Init(dxcommon_, this);
		scene_->Initialize();
		isChange_ = false;
	}
}
