#include "Engine/Core/App/MyWindow.h"

#include <cmath>

#include "imgui_impl_win32.h"

#pragma comment(lib,"winmm.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace Core;

namespace {
	MyWin::GameView gameView{ 0.0f, 0.0f, static_cast<float>(MyWin::kWindowWidth), static_cast<float>(MyWin::kWindowHeight), false, false };
	// gameView は FitGameView で毎フレーム作り直されるので別に持つ
	bool isGameViewCaptured = false;
}

namespace {
	void Fit(float x, float y, float width, float height, bool isOnPanel);
}

void MyWin::ResetGameView() {
	const MyWin* win = GetInstance();
	Fit(0.0f, 0.0f, static_cast<float>(win->clientWidth_), static_cast<float>(win->clientHeight_), false);
}

void MyWin::FitGameView(float x, float y, float width, float height) {
	Fit(x, y, width, height, true);
}

namespace {
void Fit(float x, float y, float width, float height, bool isOnPanel) {
	if (width <= 0.0f || height <= 0.0f) { return; }

	const float aspect = static_cast<float>(MyWin::kWindowWidth) / static_cast<float>(MyWin::kWindowHeight);
	float fitWidth = width;
	float fitHeight = fitWidth / aspect;
	if (fitHeight > height) {
		fitHeight = height;
		fitWidth = fitHeight * aspect;
	}
	// 余った分は上下（または左右）に均等に振って中央へ寄せる。
	// 整数ピクセルに揃えないと、貼った画像が半端にずれてぼやける
	gameView = {
		std::floor(x + (width - fitWidth) * 0.5f),
		std::floor(y + (height - fitHeight) * 0.5f),
		std::floor(fitWidth),
		std::floor(fitHeight),
		isOnPanel,
		false
	};
}
}

void MyWin::SetGameViewHovered(bool isHovered) {
	gameView.isHovered = isHovered;
}

void MyWin::SetGameViewCaptured(bool isCaptured) {
	isGameViewCaptured = isCaptured;
}

bool MyWin::IsGameViewCaptured() {
	return isGameViewCaptured;
}

const MyWin::GameView& MyWin::GetGameView() {
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
			// デバッグGUIの無い構成では、ここ以外にゲーム画面の矩形を決め直す所が無い
			ResetGameView();
		}
		return 0;

	case WM_KEYDOWN:
		// 押しっぱなしの繰り返し(bit30)では切り替えない
		if (wparam == VK_F11 && (lparam & (1 << 30)) == 0) {
			GetInstance()->ToggleFullscreen();
			return 0;
		}
		break;

	case WM_SYSKEYDOWN:
		// Alt+Enter。DXGI 側の切り替えは止めてある(SwapChainManager)ので、ここで受ける
		if (wparam == VK_RETURN && (lparam & (1 << 29)) != 0 && (lparam & (1 << 30)) == 0) {
			GetInstance()->ToggleFullscreen();
			return 0;
		}
		break;

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


void MyWin::SetFullscreen(bool isFullscreen) {
	if (isFullscreen == isFullscreen_ || !hwnd_) {
		return;
	}
	isFullscreen_ = isFullscreen;

	if (isFullscreen) {
		GetWindowPlacement(hwnd_, &windowedPlacement_);

		MONITORINFO monitor{ sizeof(MONITORINFO) };
		GetMonitorInfo(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitor);
		const RECT& rect = monitor.rcMonitor;

		SetWindowLongPtr(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(hwnd_, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
	} else {
		SetWindowLongPtr(hwnd_, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPlacement(hwnd_, &windowedPlacement_);
		SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

void MyWin::ThrowAwayWindow() {
	if (hwnd_) {
		DestroyWindow(hwnd_);
		hwnd_ = nullptr;
	}
	UnregisterClass(wc_.lpszClassName, wc_.hInstance);
	CoUninitialize();
}
