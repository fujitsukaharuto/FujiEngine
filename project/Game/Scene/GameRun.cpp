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
	textureManager_->Load("BlueprintBackground.png");
#pragma endregion


#pragma region オブジェクト読み込み
	modelManager_->LoadOBJ("suzanne.obj");
	modelManager_->LoadOBJ("Fence.obj");
	modelManager_->LoadOBJ("terrain.obj");
	modelManager_->LoadGLTF("AnimatedCube.gltf");
	modelManager_->LoadGLTF("walk.gltf");
	modelManager_->LoadGLTF("sneakWalk.gltf");
	modelManager_->LoadGLTF("simpleSkin.gltf");
#pragma endregion


#pragma region パーティクル生成
	pManager_->CreateParticleGroup("sphere", "defaultParticle.png");
	pManager_->CreateParticleGroup("sphere2", "defaultParticle.png");
	pManager_->CreateParticleGroup("sphere3", "white.png");
	pManager_->CreateParticleGroup("ring", "gradationLine.png");
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
