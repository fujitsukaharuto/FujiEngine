#include "SceneManager.h"
#include <cassert>
#include "FPSKeeper.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "ResultScene.h"
#include "ImGuiManager.h"


SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

void SceneManager::Initialize() {

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
		finishTime = 80.0f;
	}
	else if (sceneName == "GAME") {
		scene_ = new GameScene();
		finishTime = 80.0f;
	}
	else if (sceneName == "RESULT") {
		scene_ = new ResultScene();
		finishTime = 80.0f;
	}
	scene_->Init();
	scene_->Initialize();

}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime) {

	assert(nextScene_ == nullptr);

	isChange_ = true;
	changeExtraTime = extraTime;
	finishTime = extraTime * 5.0f;
	if (finishTime == 0.0f) {
		finishTime = 60.0f;
	}

	isFinifh_ = true;;


	nextScene_ = CreateScene(sceneName);

}

BaseScene* SceneManager::CreateScene(const std::string& sceneName) {

	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene();
		nowScene = sceneName;
	}
	else if (sceneName == "GAME") {
		newScene = new GameScene();
		nowScene = sceneName;
	}
	else if (sceneName == "RESULT") {
		newScene = new ResultScene();
		nowScene = sceneName;
	}

	return newScene;
}

void SceneManager::SetClear(bool is) {
	Clear = is;
	gameover = !is;
}

void SceneManager::SetGameOver(bool is) {
	gameover = is;
	Clear = !is;
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
