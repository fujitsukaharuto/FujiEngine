#include "WinApp.h"



WinApp* WinApp::GetInstance() {
	static WinApp instance;
	return &instance;
}

void WinApp::Initialize() {

	CreatGWindow(L"フジツカ", kWindowSizeX, kWindowSizeY);

}

void WinApp::Finalize() {

	ThrowAwayWindow();

}

bool WinApp::ProcessMessage() {
	MSG msg{};

	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message == WM_QUIT) {
		return true;
	}
	return false;
}

LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	
	
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対応して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::CreatGWindow(const wchar_t* title, uint32_t windowSizeX, uint32_t windowSizeY) {

	CoInitializeEx(0, COINIT_MULTITHREADED);

	//ウィンドウプロシージャ
	wc_.lpfnWndProc = (WNDPROC)WindowProc;
	//ウィンドウクラス名
	wc_.lpszClassName = L"Engine";
	//インスタンスハンドル
	wc_.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//ウィンドウクラスを登録する
	RegisterClass(&wc_);


	wrc_ = { 0,0,static_cast<int>(windowSizeX),static_cast<int>(windowSizeY) };
	AdjustWindowRect(&wrc_, WS_OVERLAPPEDWINDOW, false);


	hwnd_ = CreateWindow(
		wc_.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc_.right - wrc_.left,
		wrc_.bottom - wrc_.top,
		nullptr,
		nullptr,
		wc_.hInstance,
		nullptr
	);

	ShowWindow(hwnd_, SW_SHOW);
}

void WinApp::ThrowAwayWindow() {

	UnregisterClass(wc_.lpszClassName, wc_.hInstance);
	CoUninitialize();
	CloseWindow(hwnd_);

}
