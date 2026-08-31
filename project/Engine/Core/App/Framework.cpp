#include "Engine/Core/App/Framework.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#ifdef _DEBUGMODE
#include "imgui_internal.h"	// DockBuilder（既定レイアウトの組み立て）
#endif // _DEBUGMODE

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
	if (input_->TriggerKey(DIK_F1)) { isDebugGuiVisible_ = !isDebugGuiVisible_; }
#endif // _DEBUGMODE

	// ゲームシーンの毎フレーム処理
	sceneManager_->Update();
	ParticleManager::GetInstance()->Update();

#ifdef _DEBUGMODE
	// ImGui受付
	imguiManager_->Begin();
	if (isDebugGuiVisible_) {
		EngineDebugGUI();
		DebugGUI();
		GlobalVariables::GetInstance()->Update();
	} else {
		MyWin::FitGameView(0.0f, 0.0f, static_cast<float>(MyWin::kWindowWidth), static_cast<float>(MyWin::kWindowHeight));
	}
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

#ifdef _DEBUGMODE
namespace {

	// 保存済みのレイアウトが無いときに組む既定の配置
	void BuildDefaultDockLayout(ImGuiID dockspaceId) {
		ImGui::DockBuilderRemoveNode(dockspaceId);
		// DockSpace は内部enum。異種enum同士の | は C++20 で不可
		ImGui::DockBuilderAddNode(dockspaceId, static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_DockSpace) | ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

		// 中央は分割せずに残す。そこがゲーム画面になる
		ImGuiID center = dockspaceId;
		ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22f, nullptr, &center);
		ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, nullptr, &center);
		// 下は多めに取る。ゲーム画面は横幅で決まるので、中央を16:9より縦長にしても大きくならない
		const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.40f, nullptr, &center);
		const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.45f, nullptr, &left);
		const ImGuiID rightBottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.35f, nullptr, &right);

		ImGui::DockBuilderDockWindow("Scene", left);
		ImGui::DockBuilderDockWindow("Camera", leftBottom);
		ImGui::DockBuilderDockWindow("Light", leftBottom);
		ImGui::DockBuilderDockWindow("Raytracing", leftBottom);
		ImGui::DockBuilderDockWindow("Object Editor", right);
		ImGui::DockBuilderDockWindow("Global Variables", rightBottom);
		ImGui::DockBuilderDockWindow("Stats", bottom);
		ImGui::DockBuilderDockWindow("GPU Particle", bottom);
		ImGui::DockBuilderDockWindow("Particle Editor", bottom);
		ImGui::DockBuilderDockWindow("GPUParticle Editor", bottom);
		ImGui::DockBuilderDockWindow("OffScreen Debug", bottom);

		ImGui::DockBuilderFinish(dockspaceId);
	}
}
#endif // _DEBUGMODE

void Framework::EngineDebugGUI() {
#ifdef _DEBUGMODE
	imguiManager_->SetFontJapanese();

	// 画面全体をドッキング先にする。中央は空のままなのでゲーム画面が見える
	const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
	if (!isDockLayoutBuilt_ || isDockLayoutReset_) {
		// imgui.ini に配置が残っていればそちらを優先し、初回と組み直し要求のときだけ組む
		const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		if (isDockLayoutReset_ || node == nullptr || node->IsEmpty()) {
			BuildDefaultDockLayout(dockspaceId);
		}
		isDockLayoutBuilt_ = true;
		isDockLayoutReset_ = false;
	}

	// ゲーム画面は中央ノードへ収める。パネルを全部閉じれば画面いっぱいに戻る
	if (const ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId)) {
		MyWin::FitGameView(centralNode->Pos.x, centralNode->Pos.y, centralNode->Size.x, centralNode->Size.y);
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("View")) {
			DebugWindows::MenuItems();
			ImGui::Separator();
			if (ImGui::MenuItem("レイアウトを初期化")) { isDockLayoutReset_ = true; }
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	dxcommon_->OffscreenDebugGUI();
	sceneManager_->ParticleGroupDebugGUI();

	if (DebugWindow stats{ "Stats" }) {
		fpsKeeper_->Debug();
		ImGui::Text("FPS(平均): %.1f", ImGui::GetIO().Framerate);
		ImGui::Text("DeltaTime: %.3f", ImGui::GetIO().DeltaTime);
		ImGui::Separator();
		ImGui::Text("PickedID    : %d", modelManager_->GetPickedID());
		ImGui::Text("PickedCoord : %d, %d", modelManager_->GetPickedCoord(0), modelManager_->GetPickedCoord(1));
	}

	if (DebugWindow scene{ "Scene" }) { sceneManager_->DebugGUI(); }
	if (DebugWindow object{ "Object Editor" }) { commandManager_->DebugGUI(); }
	if (DebugWindow camera{ "Camera" }) { cameraManager_->DebugGUI(); }
	if (DebugWindow light{ "Light" }) { lightManager_->DebugGUI(); }
	if (DebugWindow raytracing{ "Raytracing" }) { objectRenderer_->DebugGUI(); }
	if (DebugWindow gpuParticle{ "GPU Particle" }) { ParticleManager::GetInstance()->ParticleCSDebugGUI(); }

	// ピックし直した瞬間だけ、編集先のウィンドウを前に出す
	const int pickedID = modelManager_->GetPickedID();
	if (pickedID > 1000 && pickedID != prevPickedID_) {
		ImGui::SetWindowFocus("Object Editor");
	}
	prevPickedID_ = pickedID;

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
