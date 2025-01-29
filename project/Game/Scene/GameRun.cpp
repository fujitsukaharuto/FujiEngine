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
	textureManager_->Load("ui.png");
	textureManager_->Load("reticle.png");
#pragma endregion


#pragma region オブジェクト読み込み
	modelManager_->LoadOBJ("suzanne.obj");
	modelManager_->LoadOBJ("playerWeapon.obj");
	modelManager_->LoadOBJ("slash2.obj");
	modelManager_->LoadOBJ("player.obj");
	modelManager_->LoadOBJ("shadow.obj");
	modelManager_->LoadOBJ("ground.obj");
	modelManager_->LoadOBJ("sky.obj");
	modelManager_->LoadOBJ("EnemyBody.obj");
	modelManager_->LoadOBJ("Colosseum.obj");

#pragma endregion


#pragma region パーティクル生成
	pManager_->CreateParticleGroup("sphere", "blueParticle.png");
	pManager_->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager_->AddAnime("animetest", "white2x2.png", 10.0f);

	// プレイヤーのパーティクル
	pManager_->CreateParticleGroup("walkParticle", "blueParticle.png", 50);

	pManager_->CreateParticleGroup("attackParticle", "blueParticle.png",40);
	pManager_->CreateParticleGroup("attackParticle2", "blueParticle.png", 40);

	pManager_->CreateParticleGroup("attackParticle3", "shockWave.png", 3);
	pManager_->CreateParticleGroup("attackParticle4", "blueParticle.png", 20);
	pManager_->CreateParticleGroup("attackParticle5", "blueParticle.png",20);


	// 敵のパーティクル
	pManager_->CreateParticleGroup("hitParticle1", "blueParticle.png",70);
	pManager_->CreateParticleGroup("hitParticle2", "blueParticle.png",50);
	pManager_->CreateParticleGroup("hitParticle3", "blueParticle.png",50);
	pManager_->CreateParticleGroup("hitParticle4", "blueParticle.png");


	pManager_->CreateParticleGroup("bomb1", "blueParticle.png", 50);
	pManager_->CreateParticleGroup("bomb2", "blueParticle.png", 50);
	pManager_->CreateParticleGroup("bomb3", "blueParticle.png", 50);

	pManager_->CreateParticleGroup("kick1", "blueParticle.png");
	pManager_->CreateParticleGroup("kick2", "blueParticle.png");
	pManager_->CreateParticleGroup("kick3", "blueParticle.png");
	pManager_->CreateParticleGroup("kick4", "blueParticle.png");

#pragma endregion


#pragma region サウンド読み込み

	audio_->LoadWave("xxx.wav");
	audio_->LoadWave("mokugyo.wav");
	audio_->LoadWave("attack1.wav");
	audio_->LoadWave("attack2.wav");
	audio_->LoadWave("jumpAttack.wav");
	audio_->LoadWave("jump01.wav");
	audio_->LoadWave("dush.wav");

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
