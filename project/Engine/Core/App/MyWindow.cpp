#include "Engine/Core/App/MyWindow.h"

#include <cmath>

#include "imgui_impl_win32.h"

#pragma comment(lib,"winmm.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace Core;

namespace {
	MyWin::ViewRect gameView{ 0.0f, 0.0f, static_cast<float>(MyWin::kWindowWidth), static_cast<float>(MyWin::kWindowHeight) };
}

void MyWin::FitGameView(float x, float y, float width, float height) {
	if (width <= 0.0f || height <= 0.0f) { return; }

	const float aspect = static_cast<float>(kWindowWidth) / static_cast<float>(kWindowHeight);
	float fitWidth = width;
	float fitHeight = fitWidth / aspect;
	if (fitHeight > height) {
		fitHeight = height;
		fitWidth = fitHeight * aspect;
	}
	// 余った分は上下（または左右）に均等に振って中央へ寄せる。
	// ビューポートは小数を取れるがシザーは整数なので、端が半端だと画面端の1行がはみ出して見える
	gameView = {
		std::floor(x + (width - fitWidth) * 0.5f),
		std::floor(y + (height - fitHeight) * 0.5f),
		std::floor(fitWidth),
		std::floor(fitHeight)
	};
}

const MyWin::ViewRect& MyWin::GetGameView() {
	return gameView;
}


MyWin* MyWin::GetInstance() {
	static MyWin instance;
	return &instance;
}


void MyWin::Initialize() {
	CreateGWindow(L"FUJI", kWindowWidth, kWindowHeight);
	timeBeginPeriod(1);
}


void MyWin::Finalize() {
	ThrowAwayWindow();
}


bool MyWin::ProcessMessage() {
	MSG msg{};

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return true;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return false;
}


LRESULT MyWin::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED) {
			GetInstance()->clientWidth_ = LOWORD(lparam);
			GetInstance()->clientHeight_ = HIWORD(lparam);
		}
		return 0;

		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対応して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void MyWin::CreateGWindow(const wchar_t* name, uint32_t windowSizeX, uint32_t windowSizeY) {
	CoInitializeEx(0, COINIT_MULTITHREADED);

	//ウィンドウプロシージャ
	wc_.lpfnWndProc = (WNDPROC)WindowProc;
	//ウィンドウクラス名
	wc_.lpszClassName = L"CG2WindowClass";
	//インスタンスハンドル
	wc_.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//ウィンドウクラスを登録する
	RegisterClass(&wc_);


	//クライアント領域のサイズ
	const int32_t kClientWidth = windowSizeX;
	const int32_t kClientHeight = windowSizeY;


	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	wrc_ = { 0,0,kClientWidth,kClientHeight };
	//クライアント領域を元に実際のサイズをwrcを変更してもらう
	AdjustWindowRect(&wrc_, WS_OVERLAPPEDWINDOW, false);

	hwnd_ = CreateWindow(
		wc_.lpszClassName,
		name,
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

	//ウィンドウを表示する
	ShowWindow(hwnd_, SW_SHOW);
}


void MyWin::ThrowAwayWindow() {
	if (hwnd_) {
		DestroyWindow(hwnd_);
		hwnd_ = nullptr;
	}
	UnregisterClass(wc_.lpszClassName, wc_.hInstance);
	CoUninitialize();
}
