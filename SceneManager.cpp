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


	darkColorL_ = { 0.175f, 0.175f, 0.175f,0.0f };
	darkColorC_ = { 0.175f, 0.175f, 0.175f,0.0f };
	darkColorR_ = { 0.175f, 0.175f, 0.175f,0.0f };

	curtainFrame_.reset(new Sprite());
	curtainFrame_->Load("curtain.png");
	curtainFrame_->SetPos({ 640,360,0.0f });
	curtainFrame_->SetSize({ 1280.0f,720.0f });

	curtainLeft_.reset(new Sprite());
	curtainLeft_->Load("curtain02.png");
	curtainLeft_->SetPos({ 650,360,0.0f });
	curtainLeft_->SetSize({ 1380.0f,760.0f });

	curtainRight_.reset(new Sprite());
	curtainRight_->Load("curtain02.png");
	curtainRight_->SetPos({ 650,360,0.0f });
	curtainRight_->SetSize({ 1380.0f,760.0f });


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


	curtainLeftPos_ = { -50.0f,375.0f,0.0f };
	curtainLeft_->SetPos(curtainLeftPos_);
	curtainRightPos_ = { 1330.0f,375.0f,0.0f };
	curtainRight_->SetPos(curtainRightPos_);


	cut.pos = { 640.0f,360.0f,0.0f,0.0f };
	cut.radius = { 0.0f,0.0f,0.0f,0.0f };


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
				isCutting_ = false;
				DXCom::GetInstance()->SetCut(false);
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

	if (isBlack_) {
		triangleL_->Draw();
		triangleC_->Draw();
		triangleR_->Draw();
	}

}

void SceneManager::StartScene(const std::string& sceneName) {

	if (sceneName == "TITLE") {
		scene_ = new TitleScene();
		finishTime = 80.0f;
		isBlack_ = true;
		isCutting_ = true;
		cuttingTime = 10.0f;
		cutMoveTime = 0.0f;
		DXCom::GetInstance()->SetCut(true);
		cut.radius.X = 0.0f;
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

	curtainTime_ = 0.0f;
	isChange_ = true;
	changeExtraTime = extraTime;
	finishTime = extraTime * 5.0f;
	if (finishTime == 0.0f) {
		finishTime = 60.0f;
	}

	isFinifh_ = true;;
	isCurtain_ = true;
	isDrawCurtain_ = true;
	maxCurtainTime_ = extraTime;

	if (sceneName == "TITLE") {
		isClose_ = true;
		isOpen_ = false;
		if (nowScene == "RESULT") {
			isCurtain_ = false;
			isClose_ = false;
			isOpen_ = false;
			isBlack_ = true;
			isDark_ = true;
			cuttingTime = 10.0f;
			cutMoveTime = 0.0f;
		}
	}
	else if (sceneName == "GAME") {
		isOpen_ = true;
		isClose_ = false;
		isCutting_ = false;
		isDark_ = false;
		changeExtraTime = 0.0f;
		DXCom::GetInstance()->SetCut(false);
		isBlack_ = false;
		if (nowScene == "RESULT") {
			isOpen_ = false;
			isClose_ = true;
			isCutting_ = false;
			isDark_ = false;
			changeExtraTime = 40.0f;
			DXCom::GetInstance()->SetCut(false);
			isBlack_ = false;
		}
	}
	else if (sceneName == "RESULT") {
		isClose_ = true;
		isOpen_ = false;
		DXCom::GetInstance()->SetCut(false);
		isBlack_ = false;
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

			curtainLeftPos_.x = Lerp(curtainLeftPos_.x, -15.0f, (0.075f) * FPSKeeper::DeltaTime());
			curtainLeft_->SetPos(curtainLeftPos_);
			curtainRightPos_.x = Lerp(curtainRightPos_.x, 1305.0f, (0.075f) * FPSKeeper::DeltaTime());
			curtainRight_->SetPos(curtainRightPos_);
		}
		else {
			isClose_ = false;
			if (nowScene == "RESULT") {
				isOpen_ = true;
				isCutting_ = false;
				isDark_ = false;
				curtainTime_ = 0.0f;
			}
			if (nowScene == "GAME") {
				isOpen_ = true;
				isCutting_ = false;
				isDark_ = false;
				curtainTime_ = 0.0f;
			}
		}
	}

}

void SceneManager::CurtainOpen() {

	if (isOpen_) {
		if (5.0f <= curtainTime_ && curtainTime_ <= maxCurtainTime_) {
			curtainTime_ += FPSKeeper::DeltaTime();

			curtainLeftPos_.x = Lerp(curtainLeftPos_.x, -720.0f, (0.075f)*FPSKeeper::DeltaTime());
			curtainLeft_->SetPos(curtainLeftPos_);
			curtainRightPos_.x = Lerp(curtainRightPos_.x, 2000.0f, (0.075f) * FPSKeeper::DeltaTime());
			curtainRight_->SetPos(curtainRightPos_);
		}
		else if (5.0f > curtainTime_) {
			curtainTime_ += FPSKeeper::DeltaTime();
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

				cuttingTime = 10.0f;
				cutMoveTime = 0.0f;
				cut.radius.X = 1000.0f;
				DXCom::GetInstance()->SetCutRadius(cut.radius.X);
			}
			else {
				isDark_ = false;
				isCutting_ = true;
				darkColorL_.W = 1.0f;
				triangleL_->SetColor(darkColorL_);

				darkColorC_.W = 1.0f;
				triangleC_->SetColor(darkColorC_);

				darkColorR_.W = 1.0f;
				triangleR_->SetColor(darkColorR_);

				cut.radius.X = 0.0f;
				DXCom::GetInstance()->SetCutRadius(cut.radius.X);

				curtainLeftPos_ = { -50.0f,375.0f,0.0f };
				curtainLeft_->SetPos(curtainLeftPos_);
				curtainRightPos_ = { 1330.0f,375.0f,0.0f };
				curtainRight_->SetPos(curtainRightPos_);
			}
		}

		CuttingUpdata();
	}

}

void SceneManager::CuttingUpdata() {


	if (isCutting_) {

		if (cuttingTime >= 0.0f) {
			cuttingTime--;
			cut.radius.X += 12.0f;
			DXCom::GetInstance()->SetCutRadius(cut.radius.X);
			cut.pos = { 640.0f,360.0f,0.0f,0.0f };
			DXCom::GetInstance()->SetCutPos(cut.pos);
			DXCom::GetInstance()->SetCut(true);

			curtainTime_ = 0.0f;
			darkColorL_.W = 0.0f;
			triangleL_->SetColor(darkColorL_);

			darkColorC_.W = 0.0f;
			triangleC_->SetColor(darkColorC_);

			darkColorR_.W = 0.0f;
			triangleR_->SetColor(darkColorR_);
		}
		else if (cutMoveTime <= 120.0f) {

			cutMoveTime += FPSKeeper::DeltaTime();

			float A = 320.0f; // 横の大きさ
			float B = 160.0f; // 縦の大きさ

			float tMove = cutMoveTime / 19.0f;
			// 時間による位置
			float x = A * sinf(tMove) + 640.0f;
			float y = B * sinf(2 * tMove) + 360.0f;

			// 描画する関数に座標を渡す
			cut.pos.X = x;
			cut.pos.Y = y;
			DXCom::GetInstance()->SetCutPos(cut.pos);

		}
		else {

			cut.radius.X += FPSKeeper::DeltaTime() * 10.0f;
			DXCom::GetInstance()->SetCutPos(cut.pos);
			DXCom::GetInstance()->SetCutRadius(cut.radius.X);
		}
	}

}
