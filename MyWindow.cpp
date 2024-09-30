#include "MyWindow.h"



#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);



MyWin* MyWin::GetInstance()
{
	static MyWin instance;
	return &instance;
}


void MyWin::Initialize() {
	CreateGWindow(L"FUJI", kWindowWidth, kWindowHeight);
}


void MyWin::Finalize() {
	ThrowAwayWindow();
}


bool MyWin::ProcessMessage() {
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


LRESULT MyWin::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対応して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void MyWin::CreateGWindow(const wchar_t* name, uint32_t windowSizeX, uint32_t windowSizeY)
{
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
	//クライアント領域を元に実際のサイズにｗｒｃを変更してもらう
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


void MyWin::ThrowAwayWindow()
{
	UnregisterClass(wc_.lpszClassName, wc_.hInstance);
	CoUninitialize();
	CloseWindow(hwnd_);

}
