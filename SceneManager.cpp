#include "SceneManager.h"
#include <cassert>
#include "FPSKeeper.h"
#include "TitleScene.h"
#include "GameScene.h"



SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
	curtainFrame_.reset();
	curtainLeft_.reset();
	curtainRight_.reset();
}

SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

void SceneManager::Initialize() {

	curtainFrame_.reset(new Sprite());
	curtainFrame_->Load("curtain.png");
	curtainFrame_->SetPos({ 640,360,0.0f });
	curtainFrame_->SetSize({ 1280.0f,720.0f });

	curtainLeft_.reset(new Sprite());
	curtainLeft_->Load("uvChecker.png");
	curtainLeft_->SetPos({ 640,360,0.0f });
	curtainLeft_->SetSize({ 1280.0f,720.0f });

	curtainRight_.reset(new Sprite());
	curtainRight_->Load("uvChecker.png");
	curtainRight_->SetPos({ 640,360,0.0f });
	curtainRight_->SetSize({ 1280.0f,720.0f });


	curtainLeftPos_ = { -640.0f,360.0f,0.0f };
	curtainLeft_->SetPos(curtainLeftPos_);
	curtainRightPos_ = { 1920.0f,360.0f,0.0f };
	curtainRight_->SetPos(curtainRightPos_);

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
			isCurtain_ = false;
		}
	}

	if (isCurtain_) {
		if (curtainTime_ <= closeCurtain_) {
			curtainTime_ += FPSKeeper::DeltaTime();

			curtainLeftPos_.x = Lerp(curtainLeftPos_.x, 30.0f, 0.1f);
			curtainLeft_->SetPos(curtainLeftPos_);
			curtainRightPos_.x = Lerp(curtainRightPos_.x, 1250.0f, 0.1f);
			curtainRight_->SetPos(curtainRightPos_);
		}
	}
	else {
		if (curtainTime_ >= 0.0f) {
			curtainTime_ -= FPSKeeper::DeltaTime();

			curtainLeftPos_.x = Lerp(curtainLeftPos_.x, -650.0f, 0.1f);
			curtainLeft_->SetPos(curtainLeftPos_);
			curtainRightPos_.x = Lerp(curtainRightPos_.x, 1930.0f, 0.1f);
			curtainRight_->SetPos(curtainRightPos_);
		}
		else {
			if (isDrawCurtain_) {
				isDrawCurtain_ = false;
			}
		}
	}


}

void SceneManager::Draw() {

	scene_->Draw();

	if (isDrawCurtain_) {
		curtainLeft_->Draw();
		curtainRight_->Draw();
	}

	curtainFrame_->Draw();

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

	isCurtain_ = true;
	isDrawCurtain_ = true;
	closeCurtain_ = extraTime;
	curtainTime_ = 0.0f;

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
