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

using namespace Core;


SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

void SceneManager::Initialize(DXCom* pDxcom, Graphics::LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
}

void SceneManager::Finalize() {
	scene_.reset();
}

void SceneManager::Update() {
	if (!isChange_) {
		if (scene_) {
			scene_->Update();
		}
		if (isFinish_) {
			finishTime -= FPSKeeper::DeltaTimeFrame();
			if (finishTime <= 0.0f) {

			}
		}
	} else {
		changeExtraTime -= FPSKeeper::DeltaTimeFrame();
		if (changeExtraTime <= 0.0f) {
			SceneSet();

		}
	}
}

void SceneManager::Draw() {
	if (scene_) {
		scene_->Draw();
	}
}

void SceneManager::StartScene(const std::string& sceneName) {
	assert(sceneFactory_);

	// Factoryから unique_ptr を受け取る
	scene_ = sceneFactory_->CreateScene(sceneName);
	scene_->Init(dxcommon_, this, lightManager_);
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

	isFinish_ = true;

	// 次のシーンを作成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}

void SceneManager::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Scene")) {
		if (scene_) {
			scene_->DebugGUI();
			ImGui::SeparatorText("Particle");
			scene_->ParticleDebugGUI();
		}
	}
#endif // _DEBUG
}

void SceneManager::ParticleGroupDebugGUI() {
#ifdef _DEBUGMODE
	if (scene_) {
		scene_->ParticleGroupDebugGUI();
	}
#endif // _DEBUG

}

void SceneManager::SceneSet() {
	if (nextScene_) {
		if (scene_) {
			ParticleManager::ParentReset();
			dxcommon_->PerFrameWait();
		}

		// 所有権を nextScene_ から scene_ へ移動
		scene_ = std::move(nextScene_);

		scene_->Init(dxcommon_, this, lightManager_);
		scene_->Initialize();
		isChange_ = false;
	}
}
