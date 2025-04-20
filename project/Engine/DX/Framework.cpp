#include "Framework.h"

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
	dxCommon_ = DXCom::GetInstance();
	dxCommon_->Initialize(win_);

	// srvManager初期化
	srvManager_ = SRVManager::GetInstance();
	srvManager_->Initialize(dxCommon_);

	// FPS管理
	fpsKeeper_ = FPSKeeper::GetInstance();
	fpsKeeper_->Initialize();

	// カメラ管理
	cameraManager_ = CameraManager::GetInstance();
	cameraManager_->Initialize();

	// ライン描画
	line3dDrawer_ = Line3dDrawer::GetInstance();
	line3dDrawer_->Initialize(dxCommon_);
	line3dDrawer_->SetCamera(cameraManager_->GetCamera());

#pragma region 汎用機能初期化
	// ImGuiの初期化
	imguiManager_ = ImGuiManager::GetInstance();
	imguiManager_->Init(win_, dxCommon_);

	// 入力の初期化
	input_ = Input::GetInstance();
	input_->Initialize();

	// オーディオの初期化
	audioPlayer_ = AudioPlayer::GetInstance();
	audioPlayer_->Initialize();

	// object関係
	textureManager_ = TextureManager::GetInstance();
	modelManager_ = ModelManager::GetInstance();
	modelManager_->Initialize(dxCommon_);

	// ライト管理
	lightManager_ = LightManager::GetInstance();
	lightManager_->Initialize(dxCommon_);
	lightManager_->CreateLight();
	lightManager_->AddPointLight();
	lightManager_->AddSpotLight();
	
	// パーティクル管理
	pManager_ = ParticleManager::GetInstance();
	pManager_->Initialize(dxCommon_, srvManager_);

#pragma endregion

	dxCommon_->SettingTexture();

	// シーン管理
	sceneManager_ = SceneManager::GetInstance();
	sceneManager_->Initialize();
}

void Framework::Run() {
	Initialize();
	while (true) {
		Update();
		if (win_->ProcessMessage()) {
			break;
		}
		Draw();
	}
	Finalize();
}
