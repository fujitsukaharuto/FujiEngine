#include "Engine/DX/Framework.h"
#include "Engine/DX/FPSKeeper.h"

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
}

void Framework::Update() {
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
