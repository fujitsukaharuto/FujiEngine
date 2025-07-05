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
	textureManager_->Load("checkerBoard.png");
	textureManager_->Load("white2x2.png");
	textureManager_->Load("BlueprintBackground.png");
	textureManager_->Load("T_Noise04.jpg");
	textureManager_->Load("Beam.png");
	textureManager_->Load("beamCore.png");
	textureManager_->Load("beamCore2.png");
	textureManager_->Load("beamCore3.png");
	textureManager_->Load("kira.png");
	textureManager_->Load("Atlas.png");
	textureManager_->Load("underRing.png");
	textureManager_->Load("boal16x16.png");
	textureManager_->Load("title_beta.png");
	textureManager_->Load("clear_beta.png");
	textureManager_->Load("gameover_beta.png");
	textureManager_->Load("spaceKey.png");
	textureManager_->Load("key_beta.png");
#pragma endregion


#pragma region オブジェクト読み込み
	ModelManager::GetInstance()->CreateSphere();
	modelManager_->LoadOBJ("cube.obj");
	modelManager_->LoadOBJ("plane_under.obj");
	modelManager_->LoadOBJ("suzanne.obj");
	modelManager_->LoadOBJ("Fence.obj");
	modelManager_->LoadOBJ("terrain.obj");
	modelManager_->LoadOBJ("skydome.obj");
	modelManager_->LoadOBJ("playerModel.obj");
	modelManager_->LoadOBJ("boss.obj");
	modelManager_->LoadOBJ("bossWaveWall.obj");
	modelManager_->LoadOBJ("Star.obj");
	modelManager_->LoadOBJ("player.obj");
	modelManager_->LoadGLTF("T_boss.gltf");
#pragma endregion


#pragma region パーティクル生成
	pManager_->CreateParticleGroup("sphere", "redCircle.png");
	pManager_->CreateParticleGroup("sphere2", "defaultParticle.png");
	pManager_->CreateParticleGroup("sphere3", "white.png");
	pManager_->CreateParticleGroup("ring", "gradationLine.png");
	pManager_->CreateParticleGroup("ChargeEffect1", "redCircle.png", 40);
	pManager_->CreateParticleGroup("ChargeEffect2", "redCircle.png", 40);
	pManager_->CreateParticleGroup("ChargeEffect3", "redCircle.png", 40);
	pManager_->CreateParticleGroup("ChargeLight", "redCircle.png", 40);
	pManager_->CreateParticleGroup("ChargeRay", "chargeRay.png", 20);
	pManager_->CreateParticleGroup("ChargeWave", "chargeCircle.png", 2);
	pManager_->CreateParticleGroup("ChargeCircle", "chargeCircle.png", 2);
	pManager_->CreateParticleGroup("BulletTrajectory", "redCircle.png", 1000);
	pManager_->CreateParticleGroup("BulletTrajectory2", "redCircle.png",1000);
	pManager_->CreateParticleGroup("playerTranjectory", "kira.png", 200);
	
	pManager_->CreateParticleGroup("bulletHit", "redCircle.png", 100);
	pManager_->CreateParticleGroup("bulletHit2", "redCircle.png", 100);
	pManager_->CreateParticleGroup("bulletHit3", "redCircle.png", 100);
	pManager_->CreateParticleGroup("bulletHitSmoke", "smoke.png", 100);
	pManager_->CreateParticleGroup("bulletHitCircle", "chargeCircle.png", 10);

	// boss
	pManager_->CreateParticleGroup("ShockRay", "chargeRay.png", 20);
	pManager_->CreateParticleGroup("ShockWaveGround", "shockWaveGround.png", 10, ShapeType::RING);
	pManager_->CreateParticleGroup("ShockWaveParticle", "redCircle.png", 30);

	pManager_->CreateParticleGroup("WaveWallSpark", "redCircle.png", 100);

	pManager_->CreateParticleGroup("BeamCharge1", "redCircle.png", 1000);
	pManager_->CreateParticleGroup("BeamCharge2", "redCircle.png", 1000);
	pManager_->CreateParticleGroup("BeamCharge3", "redCircle.png", 1000);
	pManager_->CreateParticleGroup("BeamCharge4", "redCircle.png", 1000);
	pManager_->CreateParticleGroup("BeamCharge5", "redCircle.png", 40);
	pManager_->CreateParticleGroup("BeamCharge6", "chargeCircle.png", 2);
	pManager_->CreateParticleGroup("BeamCharge7", "chargeRay.png", 20);
	pManager_->CreateParticleGroup("BeamCharge8", "redCircle.png", 20);
	pManager_->CreateParticleGroup("BeamCharge9", "redCircle.png", 40);
	pManager_->CreateParticleGroup("BeamCharge10", "redCircle.png", 10);
	pManager_->CreateParticleGroup("BeamCharge11", "redCircle.png", 40, ShapeType::RING);
	pManager_->CreateParticleGroup("BeamParticle", "redCircle.png", 200);
	pManager_->CreateParticleGroup("BeamLight", "redCircle.png", 40);



	// playerBullet用
	pManager_->CreateParentParticleGroup("ChargeEffect1", "redCircle.png",40);
	pManager_->CreateParentParticleGroup("ChargeEffect2", "redCircle.png",40);
	pManager_->CreateParentParticleGroup("ChargeEffect3", "redCircle.png",40);
	pManager_->CreateParentParticleGroup("ChargeLight", "redCircle.png",40);
	pManager_->CreateParentParticleGroup("ChargeRay", "chargeRay.png", 20);
	pManager_->CreateParentParticleGroup("ChargeWave", "chargeCircle.png", 10);
	pManager_->CreateParentParticleGroup("ChargeCircle", "chargeCircle.png", 10);


	pManager_->CreateParentParticleGroup("BeamParticle", "redCircle.png", 200);
	pManager_->CreateParentParticleGroup("BeamLight", "redCircle.png", 40);


	// 半透明になる
	pManager_->CreateParticleGroup("ShockWave", "white.png", 10, ShapeType::SPHERE);
	pManager_->CreateParticleGroup("JumpShockWave", "white.png", 10, ShapeType::SPHERE);


	pManager_->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager_->AddAnime("animetest", "white2x2.png", 10.0f);
#pragma endregion


#pragma region サウンド読み込み

	audioPlayer_->LoadWave("xxx.wav");
	audioPlayer_->LoadWave("mokugyo.wav");

#pragma endregion

#ifdef _DEBUG
	imguiManager_->InitNodeTexture();
#endif // _DEBUG


	GlobalVariables::GetInstance()->LoadFiles();

#pragma endregion

	sceneFactory_ = std::make_unique<SceneFactory>();
	sceneManager_->SetFactory(sceneFactory_.get());
	sceneManager_->StartScene("TITLE");
}

void GameRun::Finalize() {
	commandManger_->Finalize();
	sceneFactory_.reset();
	sceneManager_->Finalize();
	audioPlayer_->Finalize();
	input_->Finalize();
	imguiManager_->Fin();
	line3dDrawer_->Finalize();
	cameraManager_->Finalize();
	pManager_->Finalize();
	textureManager_->Finalize();
	modelManager_->Finalize();
	lightManager_->Finalize();
	srvManager_->Finalize();
	dxcommon_->Finalize();

	// ゲームウィンドウの破棄
	win_->Finalize();
}

void GameRun::Update() {
	fpsKeeper_->Update();

	// 入力関連の毎フレーム処理
	input_->Update();
	cameraManager_->Update();
	dxcommon_->OffscreenUpDate();

#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)) {
		if (cameraManager_->GetDebugMode()) {
			cameraManager_->SetDebugMode(false);
		} else {
			cameraManager_->SetDebugMode(true);
		}
	}

#endif // _DEBUG

	// ゲームシーンの毎フレーム処理
	sceneManager_->Update();

#ifdef _DEBUG
	// ImGui受付
	imguiManager_->Begin();
	DebugGUI();
	GlobalVariables::GetInstance()->Update();
	// ImGui受付
	imguiManager_->End();
	commandManger_->CheckInputForUndoRedo();
#endif // _DEBUG
}

void GameRun::Draw() {

	// 描画開始
	dxcommon_->PreDraw();
	// ゲームシーンの描画
	sceneManager_->Draw();
	dxcommon_->Command();
	dxcommon_->PostEffect();
	// ImGuiの描画
	imguiManager_->Draw();
	// 描画終了
	dxcommon_->PostDraw();

}

void GameRun::DebugGUI() {
#ifdef _DEBUG
	dxcommon_->OffscreenDebugGUI();
	sceneManager_->ParticleGroupDebugGUI();

	ImGui::Begin("SceneDebug");

	fpsKeeper_->Debug();
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("DeltaTime: %.3f", ImGui::GetIO().DeltaTime);

	ImGui::Separator();
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
	if (ImGui::BeginTabBar("SceneDebug", tab_bar_flags)) {
		if (ImGui::BeginTabItem("Scene")) {
			sceneManager_->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("EditorObject")) {
			commandManger_->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Camera")) {
			cameraManager_->GetCamera()->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Light")) {
			lightManager_->DebugGUI();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
#endif // _DEBUG
}
