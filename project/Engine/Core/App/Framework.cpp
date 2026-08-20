#include "Engine/Core/App/Framework.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Camera/CameraManager.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Editor;
using namespace Scene;
using namespace DXC;


Framework::Framework() {
}

void Framework::Initialize() {
}

void Framework::Finalize() {
	dxcommon_->Flush();

	// 破棄順は生成順の逆。ここを崩すと解放済みのデバイスを触る
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

void Framework::Update() {
	BeginUpdate();

#ifdef _DEBUGMODE
	if (input_->TriggerKey(DIK_F12)) {
		cameraManager_->SetDebugMode(!cameraManager_->GetDebugMode());
	}
#endif // _DEBUGMODE

	// ゲームシーンの毎フレーム処理
	sceneManager_->Update();
	ParticleManager::GetInstance()->Update();

#ifdef _DEBUGMODE
	// ImGui受付
	imguiManager_->Begin();
	EngineDebugGUI();
	DebugGUI();
	GlobalVariables::GetInstance()->Update();
	// ImGui受付
	imguiManager_->End();
	commandManager_->CheckInputForUndoRedo();
#endif // _DEBUGMODE
}

void Framework::Draw() {
	// PickingのUAVのDepthを初期化
	modelManager_->PickingDataReset();

	// 描画開始
	dxcommon_->PreDraw();
	// ゲームシーンの描画
	sceneManager_->Draw();

	// シーンが積んだものをここでまとめて描く。この順序に意味があるので入れ替えないこと
	objectRenderer_->Skinning();
	spriteRenderer_->RenderForeground();
	objectRenderer_->RenderSkyBox();
	objectRenderer_->Render();
	ParticleManager::GetInstance()->Draw();
#ifdef _DEBUGMODE
	Line3dDrawer::GetInstance()->Render();
#endif // _DEBUGMODE
	spriteRenderer_->Render();

	modelManager_->PickingDataCopy();

	dxcommon_->Command();
	dxcommon_->PostEffect();
	// ImGuiの描画
	imguiManager_->Draw();
	// 描画終了
	dxcommon_->PostDraw();
}

void Framework::EngineDebugGUI() {
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
			objectRenderer_->DebugGUI();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ParticleManager::GetInstance()->ParticleCSDebugGUI();
	ImGui::Text("%d", modelManager_->GetPickedID());
	ImGui::Text("%d,%d", modelManager_->GetPickedCoord(0), modelManager_->GetPickedCoord(1));

	ImGui::End();

	imguiManager_->UnSetFont();
#endif // _DEBUGMODE
}

void Framework::Init() {

	// ゲームウィンドウの作成
	win_ = MyWin::GetInstance();
	win_->Initialize();

	// DirectX初期化
	dxcommon_ = std::make_unique<DXCom>();
	dxcommon_->Initialize(win_);

	// srvManager初期化
	srvManager_ = SRVManager::GetInstance();
	srvManager_->Initialize(dxcommon_.get());

	// FPS管理
	fpsKeeper_ = FPSKeeper::GetInstance();
	fpsKeeper_->Initialize();

	// カメラ管理
	cameraManager_ = CameraManager::GetInstance();
	cameraManager_->Initialize(dxcommon_.get());

	// ライン描画
	line3dDrawer_ = Line3dDrawer::GetInstance();
	line3dDrawer_->Initialize(dxcommon_.get());
	line3dDrawer_->SetCamera(cameraManager_->GetCamera());

#pragma region 汎用機能初期化
	
	InitGeneralSystems();

#pragma endregion

	dxcommon_->SettingTexture();

	// シーン管理
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->Initialize(dxcommon_.get(), lightManager_.get());

	commandManager_ = CommandManager::GetInstance();
}

void Core::Framework::BeginUpdate() {
	dxcommon_->BeginFrame();
	fpsKeeper_->Update();

	// 入力関連の毎フレーム処理
	input_->Update();
	cameraManager_->Update();
	dxcommon_->OffscreenUpdate();
	modelManager_->PickingUpdate();
	lightManager_->Update();
	sceneManager_->SceneSet();
}

void Framework::Run() {
	// エンジンの初期化まで持つ。ゲーム側の Initialize() はゲームの都合だけ書けばよい
	Init();
	Initialize();
	while (true) {
		if (win_->ProcessMessage()) {
			break;
		}
		Update();
		Draw();
	}
	Finalize();
}

void Core::Framework::InitGeneralSystems() {
	// ImGuiの初期化
	imguiManager_ = ImGuiManager::GetInstance();
	imguiManager_->Initialize(win_, dxcommon_.get());

	// 入力の初期化
	input_ = Input::GetInstance();
	input_->Initialize();

	// オーディオの初期化
	audioPlayer_ = AudioPlayer::GetInstance();
	audioPlayer_->Initialize();

	// ライト管理
	lightManager_ = std::make_unique<Graphics::LightManager>();
	lightManager_->Initialize(dxcommon_.get());
	lightManager_->CreateLight();
	lightManager_->AddPointLight();

	// object関係
	textureManager_ = TextureManager::GetInstance();
	textureManager_->Initialize(dxcommon_.get());
	modelManager_ = ModelManager::GetInstance();
	modelManager_->Initialize(dxcommon_.get(), lightManager_.get());
	objectRenderer_ = ObjectRenderer::GetInstance();
	objectRenderer_->Initialize(dxcommon_.get(), lightManager_.get());
	spriteRenderer_ = SpriteRenderer::GetInstance();
	spriteRenderer_->Initialize(dxcommon_.get());

	// パーティクル管理
	pManager_ = ParticleManager::GetInstance();
	pManager_->Initialize(dxcommon_.get(), srvManager_);
}
