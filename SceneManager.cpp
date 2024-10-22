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
	curtainFrame_.reset();
	curtainLeft_.reset();
	curtainRight_.reset();
}

SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

void SceneManager::Initialize() {

	darkColorL_ = { 1.0f,1.0f,1.0f,0.0f };
	darkColorC_ = { 1.0f,1.0f,1.0f,0.0f };
	darkColorR_ = { 1.0f,1.0f,1.0f,0.0f };

	curtainFrame_.reset(new Sprite());
	curtainFrame_->Load("curtain.png");
	curtainFrame_->SetPos({ 640,360,0.0f });
	curtainFrame_->SetSize({ 1280.0f,720.0f });

	curtainLeft_.reset(new Sprite());
	curtainLeft_->Load("curtain02.png");
	curtainLeft_->SetPos({ 640,360,0.0f });
	curtainLeft_->SetSize({ 1280.0f,745.0f });

	curtainRight_.reset(new Sprite());
	curtainRight_->Load("curtain02.png");
	curtainRight_->SetPos({ 640,360,0.0f });
	curtainRight_->SetSize({ 1280.0f,745.0f });


	triangleL_.reset(new Sprite());
	triangleL_->Load("triangle.png");
	triangleL_->SetPos({ 220,100,0.0f });
	triangleL_->SetSize({ 1000.0f,1500.0f });
	triangleL_->SetColor(darkColorL_);

	triangleC_.reset(new Sprite());
	triangleC_->Load("triangle.png");
	triangleC_->SetPos({ 640,100,0.0f });
	triangleC_->SetSize({ 1000.0f,1500.0f });
	triangleC_->SetColor(darkColorC_);

	triangleR_.reset(new Sprite());
	triangleR_->Load("triangle.png");
	triangleR_->SetPos({ 1060,100,0.0f });
	triangleR_->SetSize({ 1000.0f,1500.0f });
	triangleR_->SetColor(darkColorR_);


	curtainLeftPos_ = { 30.0f,385.0f,0.0f };
	curtainLeft_->SetPos(curtainLeftPos_);
	curtainRightPos_ = { 1250.0f,385.0f,0.0f };
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


	triangleL_->Draw();
	triangleC_->Draw();
	triangleR_->Draw();

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
				float maxthree = maxCurtainTime_ / 5.0f;
				
				if (curtainTime_ >= maxthree) {
					darkColorL_.W = 1.0f;
					triangleL_->SetColor(darkColorL_);
				}

				if (curtainTime_ >= maxthree * 2.0f) {
					darkColorR_.W = 1.0f;
					triangleR_->SetColor(darkColorR_);
				}

				if (curtainTime_ >= maxthree * 3.0f) {
					darkColorC_.W = 1.0f;
					triangleC_->SetColor(darkColorC_);
				}


			}
			else {
				isDark_ = false;
				darkColorL_.W = 1.0f;
				triangleL_->SetColor(darkColorL_);

				darkColorC_.W = 1.0f;
				triangleC_->SetColor(darkColorC_);

				darkColorR_.W = 1.0f;
				triangleR_->SetColor(darkColorR_);
			}
		}
		else {
			if (curtainTime_ >= 0.0f) {
				curtainTime_ -= FPSKeeper::DeltaTime();
				float maxthree = maxCurtainTime_ / 5.0f;

				if (curtainTime_ <= maxthree * 3.0f) {
					darkColorC_.W = 0.0f;
					triangleC_->SetColor(darkColorC_);
				}

				if (curtainTime_ <= maxthree * 2.0f) {
					darkColorR_.W = 0.0f;
					triangleR_->SetColor(darkColorR_);
				}

				if (curtainTime_ <= maxthree) {
					darkColorL_.W = 0.0f;
					triangleL_->SetColor(darkColorL_);
				}

			}
			else {
				darkColorL_.W = 0.0f;
				triangleL_->SetColor(darkColorL_);

				darkColorC_.W = 0.0f;
				triangleC_->SetColor(darkColorC_);

				darkColorR_.W = 0.0f;
				triangleR_->SetColor(darkColorR_);
			}
		}
	}

}
