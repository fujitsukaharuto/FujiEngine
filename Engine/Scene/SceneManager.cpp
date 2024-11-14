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
	black.reset(new Sprite);
	black->Load("white2x2.png");
	black->SetSize({ 2560.0f,720.0f });
	black->SetPos({ 2560.0f,360.0f,0.0f });
	black->SetColor({ 0.0f,0.0f,0.0f,1.0f });

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
				isBlack = false;
				blackXpos = 2560.0f;
				isblackLeft = false;
				finishTime = 0.0f;
				isFinifh_ = false;
			}
		}
	}
	else {

		changeExtraTime -= FPSKeeper::DeltaTime();
		if (changeExtraTime <= 0.0f) {
			SceneSet();
			isBlack = false;
		}
	}


	if (isBlack) {

		blackXpos= Lerp(blackXpos, -10.0f, (0.075f) * FPSKeeper::DeltaTime());
		black->SetPos({ blackXpos,360.0f ,0.0f });
	}
	if (isblackLeft) {
		blackXpos = Lerp(blackXpos, -2570.0f, (0.075f));
		black->SetPos({ blackXpos,360.0f ,0.0f });
	}

}

void SceneManager::Draw() {

	scene_->Draw();

	if (isBlack||isblackLeft) {
		black->Draw();
	}

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

	isFinifh_ = true;
	isBlack = true;
	blackXpos = 2560.0f;

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
		isblackLeft = true;
	}

}
