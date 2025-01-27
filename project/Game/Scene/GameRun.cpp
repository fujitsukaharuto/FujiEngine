#include "GameRun.h"
#include "SceneFactory.h"

GameRun::GameRun() {
}

GameRun::~GameRun() {
}

void GameRun::Initialize() {
	Init();

	// リソースをここであらかじめLoadしておく
#pragma region リソース読み込み

#pragma region テクスチャ読み込み
	textureManager_->Load("uvChecker.png");
	textureManager_->Load("title.png");
	textureManager_->Load("clear.png");
	textureManager_->Load("key.png");
	textureManager_->Load("uiHome.png");
	textureManager_->Load("menu_button1.png");
	textureManager_->Load("menu_button2.png");
#pragma endregion


#pragma region オブジェクト読み込み
	modelManager_->LoadOBJ("suzanne.obj");
	modelManager_->LoadOBJ("Fence.obj");
	modelManager_->LoadOBJ("terrain.obj");
	modelManager_->LoadOBJ("ufo_test.obj");
	modelManager_->LoadOBJ("ufo_light.obj");
	modelManager_->LoadOBJ("FieldBlock.obj");
	modelManager_->LoadOBJ("daungerousBlock.obj");
	modelManager_->LoadOBJ("NormalEnemy.obj");
	modelManager_->LoadOBJ("StrongEnemy.obj");
#pragma endregion


#pragma region パーティクル生成
	pManager_->CreateParticleGroup("sphere", "blueParticle.png");
	pManager_->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager_->AddAnime("animetest", "white2x2.png", 10.0f);

	pManager_->CreateParticleGroup("bomb1", "blueParticle.png");
	pManager_->CreateParticleGroup("bomb2", "blueParticle.png");
	pManager_->CreateParticleGroup("bomb3", "blueParticle.png");

	pManager_->CreateParticleGroup("kick1", "blueParticle.png");
	pManager_->CreateParticleGroup("kick2", "blueParticle.png");
	pManager_->CreateParticleGroup("kick3", "blueParticle.png");
	pManager_->CreateParticleGroup("kick4", "blueParticle.png");

	pManager_->CreateParticleGroup("hardKick1", "blueParticle.png");
	pManager_->CreateParticleGroup("hardKick2", "blueParticle.png");
	pManager_->CreateParticleGroup("hardKick3", "blueParticle.png");
	pManager_->CreateParticleGroup("hardKick4", "blueParticle.png");

	pManager_->CreateParticleGroup("flyEnemy1", "blueParticle.png", 30);
	pManager_->CreateParticleGroup("flyEnemy2", "blueParticle.png", 30);

	pManager_->CreateParticleGroup("charge1", "blueParticle.png");
	pManager_->CreateParticleGroup("charge2", "blueParticle.png");

	pManager_->CreateParticleGroup("kiran1", "blueParticle.png");
	pManager_->CreateParticleGroup("kiran2", "blueParticle.png");

	pManager_->CreateParticleGroup("star1", "blueParticle.png");
	pManager_->CreateParticleGroup("star2", "kira.png");

#pragma endregion


#pragma region サウンド読み込み

	audio_->LoadWave("xxx.wav");
	audio_->LoadWave("mokugyo.wav");
	audio_->LoadWave("kickAir.wav");
	audio_->LoadWave("kickHit.wav");
	audio_->LoadWave("kickCharge.wav");
	audio_->LoadWave("mini_bomb2.wav");
	audio_->LoadWave("jump.wav");


#pragma endregion



	GlobalVariables::GetInstance()->LoadFiles();

#pragma endregion

	sceneFactory_ = std::make_unique<SceneFactory>();
	sceneManager_->SetFactory(sceneFactory_.get());
	sceneManager_->StartScene("TITLE");
}

void GameRun::Finalize() {
	sceneFactory_.reset();
	sceneManager_->Finalize();
	audio_->Finalize();
	imguiManager_->Fin();
	pManager_->Finalize();
	textureManager_->Finalize();
	modelManager_->Finalize();
	srvManager_->Finalize();

	// ゲームウィンドウの破棄
	win_->Finalize();
}

void GameRun::Update() {
	fpsKeeper_->Update();
#ifdef _DEBUG
	// ImGui受付
	imguiManager_->Begin();
#endif // _DEBUG

	// 入力関連の毎フレーム処理
	input_->Update();
	cameraManager_->Update();

#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)) {
		if (cameraManager_->GetDebugMode()) {
			cameraManager_->SetDebugMode(false);
		}
		else {
			cameraManager_->SetDebugMode(true);
		}
	}
#endif // _DEBUG

	GlobalVariables::GetInstance()->Update();
	// ゲームシーンの毎フレーム処理
	sceneManager_->Update();
	// ImGui受付
	imguiManager_->End();
}

void GameRun::Draw() {

	// 描画開始
	dxCommon_->PreDraw();
	// ゲームシーンの描画
	sceneManager_->Draw();
	dxCommon_->Command();
	dxCommon_->PostEffect();
	// ImGuiの描画
	imguiManager_->Draw();
	// 描画終了
	dxCommon_->PostDraw();

}
