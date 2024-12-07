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

	srvManager_ = SRVManager::GetInstance();
	srvManager_->Initialize();

	fpsKeeper_ = FPSKeeper::GetInstance();
	fpsKeeper_->Initialize();

	cameraManager_ = CameraManager::GetInstance();
	cameraManager_->Initialize();

	line3dDrawer_ = Line3dDrawer::GetInstance();
	line3dDrawer_->Initialize();
	line3dDrawer_->SetCamera(cameraManager_->GetCamera());

#pragma region 汎用機能初期化
	// ImGuiの初期化
	imguiManager_ = ImGuiManager::GetInstance();
	imguiManager_->Init(win_, dxCommon_);

	// 入力の初期化
	input_ = Input::GetInstance();
	input_->Initialize();

	// オーディオの初期化
	audio_ = Audio::GetInstance();
	audio_->Initialize();

	textureManager_ = TextureManager::GetInstance();
	modelManager_ = ModelManager::GetInstance();

	pointLightManager_ = PointLightManager::GetInstance();
	pointLightManager_->AddPointLight();
	pointLightManager_->AddSpotLight();

	pManager_ = ParticleManager::GetInstance();
	pManager_->Initialize(dxCommon_, srvManager_);

#pragma endregion

	dxCommon_->SettingTexture();


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
