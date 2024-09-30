#include "Audio.h"
#include "DXCom.h"
#include "GameScene.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "MyWindow.h"



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	MyWin* win = nullptr;
	DXCom* dxCommon = nullptr;
	// 汎用
	Input* input = nullptr;
	Audio* audio = nullptr;
	GameScene* gameScene = nullptr;
	TextureManager* textureManager = nullptr;

	// ゲームウィンドウの作成
	win = MyWin::GetInstance();
	win->Initialize();

	// DirectX初期化
	dxCommon = DXCom::GetInstance();
	dxCommon->InitDX(win);


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

	textureManager->GetInstance();

#pragma endregion

	dxCommon->SettingTexture();


	gameScene = new GameScene();
	gameScene->Initialize();


	//BYTE keys[256] = { 0 };
	//BYTE preKeys[256] = { 0 };

	//ウィンドウのxボタンが押されるまでループ
	while (true)
	{
		if (win->ProcessMessage())
		{
			break;
		}

#ifdef _DEBUG
		// ImGui受付
		imguiManager->Begin();
#endif // _DEBUG

		// 入力関連の毎フレーム処理
		input->Update();
		// ゲームシーンの毎フレーム処理
		gameScene->Update();

#ifdef _DEBUG

#endif // _DEBUG

		// ImGui受付
		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();
		// ゲームシーンの描画
		gameScene->Draw();
		// ImGuiの描画
		imguiManager->Draw();
		// 描画終了
		dxCommon->PostDraw();
	}

	// 解放
	delete gameScene;
	
	audio->Finalize();
	imguiManager->Fin();
	textureManager->Finalize();

	// ゲームウィンドウの破棄
	win->Finalize();

	return 0;
}

