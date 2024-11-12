#include "Audio.h"
#include "DXCom.h"
#include "GameScene.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "SRVManager.h"
#include "MyWindow.h"
#include "GlobalVariables.h"
#include "ModelManager.h"
#include "PointLightManager.h"
#include "CameraManager.h"
#include "Line3dDrawer.h"
#include "ParticleManager.h"
#include "Scene/SceneManager.h"

// やること
// オフスクリーンのかんり
// モデルマネージャー、スプライトマネージャー
// wave対応
// クラス分け



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	MyWin* win = nullptr;
	DXCom* dxCommon = nullptr;
	SRVManager* srvManager = nullptr;
	// 汎用
	Input* input = nullptr;
	Audio* audio = nullptr;
	FPSKeeper* fpsKeeper = nullptr;
	SceneManager* sceneManager = nullptr;
	TextureManager* textureManager = nullptr;
	ModelManager* modelManager = nullptr;

	PointLightManager* pointLightManager = nullptr;
	CameraManager* cameraManager = nullptr;
	DebugCamera* camera = nullptr;
	Line3dDrawer* line3dDrawer = nullptr;
	ParticleManager* pManager = nullptr;


	// ゲームウィンドウの作成
	win = MyWin::GetInstance();
	win->Initialize();

	// DirectX初期化
	dxCommon = DXCom::GetInstance();
	dxCommon->Initialize(win);

	srvManager = SRVManager::GetInstance();
	srvManager->Initialize();

	fpsKeeper = FPSKeeper::GetInstance();
	fpsKeeper->Initialize();

	camera = DebugCamera::GetInstance();
	camera->Initialize();


	cameraManager = CameraManager::GetInstance();
	cameraManager->Initialize();


	line3dDrawer = Line3dDrawer::GetInstance();
	line3dDrawer->Initialize();
	line3dDrawer->SetCamera(cameraManager->GetCamera());


#pragma region 汎用機能初期化
	// ImGuiの初期化
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();
	imguiManager->Init(win,dxCommon);

	// 入力の初期化
	input = Input::GetInstance();
	input->Initialize();

	// オーディオの初期化
	audio = Audio::GetInstance();
	audio->Initialize();

	textureManager = TextureManager::GetInstance();
	modelManager = ModelManager::GetInstance();

	pointLightManager = PointLightManager::GetInstance();
	pointLightManager->AddPointLight();
	pointLightManager->AddSpotLight();


	pManager = ParticleManager::GetInstance();
	pManager->Initialize(dxCommon, srvManager);
	pManager->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager->AddAnime("animetest", "white2x2.png", 10.0f);

#pragma endregion

	dxCommon->SettingTexture();

	GlobalVariables::GetInstance()->LoadFiles();



	textureManager->Load("Sprite/0.png");
	textureManager->Load("Sprite/1.png");
	textureManager->Load("Sprite/2.png");
	textureManager->Load("Sprite/3.png");
	textureManager->Load("Sprite/4.png");
	textureManager->Load("Sprite/5.png");
	textureManager->Load("Sprite/6.png");
	textureManager->Load("Sprite/7.png");
	textureManager->Load("Sprite/8.png");
	textureManager->Load("Sprite/9.png");




	sceneManager = SceneManager::GetInstance();
	sceneManager->Initialize();
	sceneManager->StartScene("GAME");



	//BYTE keys[256] = { 0 };
	//BYTE preKeys[256] = { 0 };

	//ウィンドウのxボタンが押されるまでループ
	while (!win->ProcessMessage())
	{

		fpsKeeper->Update();

#ifdef _DEBUG
		// ImGui受付
		imguiManager->Begin();
#endif // _DEBUG

		// 入力関連の毎フレーム処理
		input->Update();

		cameraManager->Update();
#ifdef _DEBUG
		if (input->TriggerKey(DIK_F12)) {
			if (cameraManager->GetDebugMode()) {
				cameraManager->SetDebugMode(false);

			}
			else {
				cameraManager->SetDebugMode(true);

			}
		}
#endif // _DEBUG

		GlobalVariables::GetInstance()->Update();

		// ゲームシーンの毎フレーム処理
		sceneManager->Update();

		// ImGui受付
		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();
		// ゲームシーンの描画
		sceneManager->Draw();
		// ImGuiの描画
		imguiManager->Draw();
		// 描画終了
		dxCommon->PostDraw();
	}

	// 解放
	sceneManager->Finalize();
	
	audio->Finalize();
	imguiManager->Fin();
	pManager->Finalize();
	textureManager->Finalize();
	modelManager->Finalize();
	srvManager->Finalize();

	// ゲームウィンドウの破棄
	win->Finalize();

	return 0;
}

