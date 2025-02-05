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
	textureManager_->Load("GameTexture/gameover.png");
	textureManager_->Load("key.png");
	textureManager_->Load("uiHome.png");
	textureManager_->Load("menu_button1.png");
	textureManager_->Load("menu_button2.png");
	textureManager_->Load("startUI.png");
	textureManager_->Load("HibiGround_color1.png");
	textureManager_->Load("HibiGround_color2.png");
#pragma endregion


#pragma region オブジェクト読み込み
	modelManager_->LoadOBJ("suzanne.obj");
	modelManager_->LoadOBJ("Fence.obj");
	modelManager_->LoadOBJ("Field.obj");
	modelManager_->LoadOBJ("SkyDome.obj");
	modelManager_->LoadOBJ("terrain.obj");
	modelManager_->LoadOBJ("player.obj");
	modelManager_->LoadOBJ("ufo_test.obj");
	modelManager_->LoadOBJ("ufo_light.obj");
	modelManager_->LoadOBJ("NormalGround.obj");
	modelManager_->LoadOBJ("DangerGround.obj");
	modelManager_->LoadOBJ("NormalEnemy.obj");
	modelManager_->LoadOBJ("StrongEnemy.obj");
	modelManager_->LoadOBJ("KickDir.obj");
	modelManager_->LoadOBJ("bakudan.obj");
	modelManager_->LoadOBJ("PlayerFoot.obj");
	modelManager_->LoadOBJ("Missile.obj");
	modelManager_->LoadOBJ("Parachute.obj");
#pragma endregion


#pragma region パーティクル生成
	pManager_->CreateParticleGroup("sphere", "blueParticle.png");
	pManager_->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager_->AddAnime("animetest", "white2x2.png", 10.0f);

	pManager_->CreateParticleGroup("bomb1", "blueParticle.png", 30);
	pManager_->CreateParticleGroup("bomb2", "blueParticle.png", 30);
	pManager_->CreateParticleGroup("bomb3", "blueParticle.png", 30);

	pManager_->CreateParticleGroup("kick1", "blueParticle.png");
	pManager_->CreateParticleGroup("kick2", "blueParticle.png");
	pManager_->CreateParticleGroup("kick3", "blueParticle.png");
	pManager_->CreateParticleGroup("kick4", "blueParticle.png");

	pManager_->CreateParticleGroup("shockWave1", "blueParticle.png", 40);
	pManager_->CreateParticleGroup("shockWave2", "blueParticle.png", 40);

	pManager_->CreateParticleGroup("bombShock1", "blueParticle.png", 40);
	pManager_->CreateParticleGroup("bombShock2", "blueParticle.png", 40);

	pManager_->CreateParticleGroup("MisilleBomb1", "blueParticle.png");
	pManager_->CreateParticleGroup("MisilleBomb2", "blueParticle.png");

	pManager_->CreateParticleGroup("hardKick1", "blueParticle.png");
	pManager_->CreateParticleGroup("hardKick2", "blueParticle.png");
	pManager_->CreateParticleGroup("hardKick3", "blueParticle.png");
	pManager_->CreateParticleGroup("hardKick4", "blueParticle.png");

	pManager_->CreateParticleGroup("flyEnemy1", "blueParticle.png", 30);
	pManager_->CreateParticleGroup("flyEnemy2", "blueParticle.png", 30);

	pManager_->CreateParticleGroup("flyPlayer1", "blueParticle.png", 30);
	pManager_->CreateParticleGroup("flyPlayer2", "blueParticle.png", 30);

	pManager_->CreateParticleGroup("charge1", "blueParticle.png");
	pManager_->CreateParticleGroup("charge2", "blueParticle.png");

	pManager_->CreateParticleGroup("chargeMax1", "kira.png");

	pManager_->CreateParticleGroup("kiran1", "blueParticle.png");
	pManager_->CreateParticleGroup("kiran2", "blueParticle.png");

	pManager_->CreateParticleGroup("star1", "blueParticle.png",40);
	pManager_->CreateParticleGroup("star2", "kira.png",40);

	pManager_->CreateParticleGroup("jump", "blueParticle.png", 15);
	pManager_->CreateParticleGroup("jumpLanding", "blueParticle.png", 15);

	pManager_->CreateParticleGroup("playerDead1", "blueParticle.png");
	pManager_->CreateParticleGroup("playerDead2", "blueParticle.png");

	pManager_->CreateParticleGroup("lifeUIBreak1", "blueParticle.png");
	pManager_->CreateParticleGroup("lifeUIBreak2", "blueParticle.png");

	pManager_->CreateParticleGroup("dark", "blueParticle.png", 30, true);

	pManager_->CreateParticleGroup("revival1", "blueParticle.png", 30);
	pManager_->CreateParticleGroup("revival2", "kira.png", 30);

	pManager_->CreateParticleGroup("misilleSmoke1", "blueParticle.png");
	pManager_->CreateParticleGroup("misilleSmoke2", "blueParticle.png");

	pManager_->CreateParticleGroup("star", "kira.png", 30);

	pManager_->CreateParticleGroup("groundCrack1", "kira.png");

	pManager_->CreateParticleGroup("groundBreak", "blueParticle.png", 80);
	pManager_->CreateParticleGroup("groundBreakReverse", "blueParticle.png",80);




#pragma endregion


#pragma region サウンド読み込み

	audio_->LoadWave("xxx.wav");
	audio_->LoadWave("mokugyo.wav");
	audio_->LoadWave("kickAir.wav");
	audio_->LoadWave("kickHit.wav");
	audio_->LoadWave("kickCharge.wav");
	audio_->LoadWave("mini_bomb2.wav");
	audio_->LoadWave("jump.wav");
	audio_->LoadWave("littleDamage.wav");
	audio_->LoadWave("bigDamage.wav");
	audio_->LoadWave("playerDead.wav");
	audio_->LoadWave("playerRevival.wav");
	audio_->LoadWave("hit_wdn_pillar3.wav");
	audio_->LoadWave("MissileBomb.wav");
	audio_->LoadWave("MissileLauncher.wav");
	audio_->LoadWave("missileSize.wav");
	audio_->LoadWave("missileApe.wav");
	audio_->LoadWave("sizeup.wav");
	audio_->LoadWave("openParachute.wav");
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
