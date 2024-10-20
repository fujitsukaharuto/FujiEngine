#include "SceneManager.h"
#include <cassert>
#include "FPSKeeper.h"
#include "TitleScene.h"
#include "GameScene.h"



SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

void SceneManager::Finalize() {
	delete scene_;
}

void SceneManager::Update() {

	if (!isChange_) {
		scene_->Update();
	}
	else {
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

	if (sceneName == "TITLE") {
		scene_ = new TitleScene();
	}
	else if (sceneName == "GAME") {
		scene_ = new GameScene();
	}
	scene_->Init();
	scene_->Initialize();

}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime) {

	assert(nextScene_ == nullptr);

	isChange_ = true;
	changeExtraTime = extraTime;

	nextScene_ = CreateScene(sceneName);

}

BaseScene* SceneManager::CreateScene(const std::string& sceneName) {

	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene();
	}
	else if (sceneName == "GAME") {
		newScene = new GameScene();
	}

	return newScene;
}

void SceneManager::SceneSet() {

	if (nextScene_) {
		if (scene_) {

			delete scene_;
		}

		scene_ = nextScene_;
		nextScene_ = nullptr;

		scene_->Init();
		scene_->Initialize();
		isChange_ = false;
	}

}
