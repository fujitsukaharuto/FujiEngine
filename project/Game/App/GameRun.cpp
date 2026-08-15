#include "GameRun.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Game/Scene/SceneFactory.h"
#include "Game/Particle/GameEmitters.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/Raytracing/RayTracedAOPass.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Editor;
using namespace Scene;


GameRun::GameRun() {
}

GameRun::~GameRun() {
}

void GameRun::Initialize() {
	Init();

	// リソースをここであらかじめLoadしておく
#pragma region リソース読み込み

#pragma region テクスチャ読み込み
	textureManager_->LoadAll();
#pragma endregion

#pragma region オブジェクト読み込み
	ModelManager::GetInstance()->CreateSphere();
	// resource/Models 内の.obj/.gltfを全て読み込む(T_boss.gltfもここに含まれる)
	modelManager_->LoadAllFileData();
#pragma endregion

#pragma region パーティクル生成
	
	LoadParticleGroup();
	Game::CreateDefaultEmitters();

#pragma endregion

#pragma region サウンド読み込み

	LoadSoundData();

#pragma endregion

	GlobalVariables::GetInstance()->LoadFiles();

#pragma endregion

	sceneFactory_ = std::make_unique<SceneFactory>();
	sceneManager_->SetFactory(sceneFactory_.get());

	int sceneNum = 0;
	if (sceneNum == 0) {
		sceneManager_->StartScene("TEST");
	} else {
		sceneManager_->StartScene("TITLE");
	}

}

void GameRun::Finalize() {
	dxcommon_->Flush();

	commandManager_->Finalize();
	sceneFactory_.reset();
	sceneManager_->Finalize();
	audioPlayer_->Finalize();
	input_->Finalize();
	imguiManager_->Fin();
	line3dDrawer_->Finalize();
	cameraManager_->Finalize();
	pManager_->Finalize();
	spriteRenderer_->Finalize();
	objectRenderer_->Finalize();
	textureManager_->Finalize();
	modelManager_->Finalize();
	lightManager_->Finalize();
	srvManager_->Finalize();
	dxcommon_->Finalize();

	// ゲームウィンドウの破棄
	win_->Finalize();
}

void GameRun::Update() {
	BeginUpdate();

#ifdef _DEBUGMODE
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
	ParticleManager::GetInstance()->Update();

#ifdef _DEBUGMODE
	// ImGui受付
	imguiManager_->Begin();
	DebugGUI();
	GlobalVariables::GetInstance()->Update();
	// ImGui受付
	imguiManager_->End();
	commandManager_->CheckInputForUndoRedo();
#endif // _DEBUG
}

void GameRun::Draw() {
	// PickingのUAVのDepthを初期化
	modelManager_->PickingDataReset();

	// 描画開始
	dxcommon_->PreDraw();
	// ゲームシーンの描画
	sceneManager_->Draw();

	objectRenderer_->Skinning();
	spriteRenderer_->RenderForeground();
	objectRenderer_->RenderSkyBox();
	objectRenderer_->Render();
	ParticleManager::GetInstance()->Draw();
#ifdef _DEBUGMODE
	Line3dDrawer::GetInstance()->Render();
#endif // _DEBUG
	spriteRenderer_->Render();

	modelManager_->PickingDataCopy();

	dxcommon_->Command();
	dxcommon_->PostEffect();
	// ImGuiの描画
	imguiManager_->Draw();
	// 描画終了
	dxcommon_->PostDraw();
}

void GameRun::DebugGUI() {
#ifdef _DEBUGMODE
	imguiManager_->SetFontJapanese();

	dxcommon_->OffscreenDebugGUI();
	sceneManager_->ParticleGroupDebugGUI();
	
	ImGui::Begin("SceneDebug");

	fpsKeeper_->Debug();
	ImGui::Text("FPS(平均): %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("DeltaTime: %.3f", ImGui::GetIO().DeltaTime);

	ImGui::Separator();
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
	if (ImGui::BeginTabBar("SceneDebug", tab_bar_flags)) {
		if (ImGui::BeginTabItem("Scene")) {
			sceneManager_->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ModelManager::GetInstance()->GetPickedID() > 1000) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		}
		if (ImGui::BeginTabItem("EditorObject")) {
			if (ModelManager::GetInstance()->GetPickedID() > 1000) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}
			commandManager_->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Camera")) {
			cameraManager_->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Light")) {
			lightManager_->DebugGUI();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Raytracing")) {
			if (auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene()) {
				rtScene->DebugGUI();
			}
			if (auto* gbuffer = ObjectRenderer::GetInstance()->GetGBufferPass()) {
				gbuffer->DebugGUI();
			}
			if (auto* aoPass = ObjectRenderer::GetInstance()->GetRayTracedAOPass()) {
				aoPass->DebugGUI();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ParticleManager::GetInstance()->ParticleCSDebugGUI();
	ImGui::Text("%d", modelManager_->GetPickedID());
	ImGui::Text("%d,%d", modelManager_->GetPickedCoord(0), modelManager_->GetPickedCoord(1));

	ImGui::End();

	imguiManager_->UnSetFont();
#endif // _DEBUG
}

void GameRun::LoadParticleGroup() {
	// パーティクルグループの生成情報は resource/ParticleGroups/*.json が持っている。
	// テクスチャ・数・形状・ペアレント版の要否まで全てJSON側で完結する
	pManager_->LoadAllFileData();

	// 連番アニメだけはJSON走査の対象外なのでここで生成する
	pManager_->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager_->AddAnime("animetest", "white2x2.png", 10.0f);
}

void GameRun::LoadSoundData() {
	audioPlayer_->LoadAll();
}
