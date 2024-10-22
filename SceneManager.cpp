#include "SceneManager.h"
#include <cassert>
#include "FPSKeeper.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "ResultScene.h"



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

	darkColor_ = { 0.0f,0.0f,0.0f,0.0f };

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

	Black_.reset(new Sprite());
	Black_->Load("white2x2.png");
	Black_->SetPos({ 640,360,0.0f });
	Black_->SetSize({ 1280.0f,720.0f });
	Black_->SetColor(darkColor_);


	curtainLeftPos_ = { 30.0f,360.0f,0.0f };
	curtainLeft_->SetPos(curtainLeftPos_);
	curtainRightPos_ = { 1250.0f,360.0f,0.0f };
	curtainRight_->SetPos(curtainRightPos_);

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
				isCurtain_ = false;
				isBlack_ = false;
				isFinifh_ = false;
			}
		}
	}
	else {
		changeExtraTime -= FPSKeeper::DeltaTime();
		if (changeExtraTime <= 0.0f) {
			SceneSet();

		}
	}

	CurtainUpdata();

	BlackUpdata();

}

void SceneManager::Draw() {

	scene_->Draw();

	if (isDrawCurtain_) {
		curtainLeft_->Draw();
		curtainRight_->Draw();
	}

	curtainFrame_->Draw();

	Black_->Draw();

}

void SceneManager::StartScene(const std::string& sceneName) {

	if (sceneName == "TITLE") {
		scene_ = new TitleScene();
	}
	else if (sceneName == "GAME") {
		scene_ = new GameScene();
	}
	else if (sceneName == "RESULT") {
		scene_ = new ResultScene();
	}
	scene_->Init();
	scene_->Initialize();

}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime) {

	assert(nextScene_ == nullptr);

	curtainTime_ = 0.0f;
	isChange_ = true;
	changeExtraTime = extraTime;
	finishTime = extraTime * 2.0f;
	if (finishTime == 0.0f) {
		finishTime = 60.0f;
	}

	isFinifh_ = true;;
	isCurtain_ = true;
	isDrawCurtain_ = true;
	maxCurtainTime_ = extraTime;

	if (sceneName == "TITLE") {
		isClose_ = true;
		if (nowScene == "RESULT") {
			isClose_ = false;
			isBlack_ = true;
			isDark_ = true;
		}
	}
	else if (sceneName == "GAME") {
		isOpen_ = true;
		changeExtraTime = 0.0f;
	}
	else if (sceneName == "RESULT") {
		isClose_ = true;
	}




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

void SceneManager::CurtainUpdata() {

	if (isCurtain_) {
		CurtainClose();
		CurtainOpen();
	}

}

void SceneManager::CurtainClose() {

	if (isClose_) {
		if (curtainTime_ <= maxCurtainTime_) {
			curtainTime_ += FPSKeeper::DeltaTime();

			curtainLeftPos_.x = Lerp(curtainLeftPos_.x, 40.0f, 0.075f);
			curtainLeft_->SetPos(curtainLeftPos_);
			curtainRightPos_.x = Lerp(curtainRightPos_.x, 1240.0f, 0.075f);
			curtainRight_->SetPos(curtainRightPos_);
		}
		else {
			isClose_ = false;

		}
	}

}

void SceneManager::CurtainOpen() {

	if (isOpen_) {
		if (curtainTime_ <= maxCurtainTime_) {
			curtainTime_ += FPSKeeper::DeltaTime();

			curtainLeftPos_.x = Lerp(curtainLeftPos_.x, -650.0f, 0.075f);
			curtainLeft_->SetPos(curtainLeftPos_);
			curtainRightPos_.x = Lerp(curtainRightPos_.x, 1930.0f, 0.075f);
			curtainRight_->SetPos(curtainRightPos_);
		}
		else {
			isOpen_ = false;

		}
	}

}

void SceneManager::BlackUpdata() {

	if (isBlack_) {
		if (isDark_) {
			if (curtainTime_ <= maxCurtainTime_) {
				curtainTime_ += FPSKeeper::DeltaTime();

				darkColor_.W = Lerp(darkColor_.W, 1.0f, 0.1f);
				Black_->SetColor(darkColor_);

			}
			else {
				isDark_ = false;
				darkColor_.W = 1.0f;
				Black_->SetColor(darkColor_);
			}
		}
		else {
			if (curtainTime_ >= 0.0f) {
				curtainTime_ -= FPSKeeper::DeltaTime();

				darkColor_.W = Lerp(darkColor_.W, 0.0f, 0.1f);
				Black_->SetColor(darkColor_);

			}
			else {
				darkColor_.W = 0.0f;
				Black_->SetColor(darkColor_);
			}
		}
	}

}
