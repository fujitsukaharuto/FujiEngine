#pragma once
#include <Windows.h>
#include <cstdint>



class WinApp {
public:
	WinApp() = default;
	~WinApp() = default;
public:

	static const int kWindowSizeX = 1280;
	static const int kWindowSizeY = 720;

public:

	static WinApp* GetInstance();

	void Initialize();

	void Finalize();

	bool ProcessMessage();


	/// ==========================================
	/// Getter
	/// ==========================================

	const WNDCLASS& getWNDCLASS() const { return wc_; }
	const RECT& GetRECT() const { return wrc_; }
	const HWND& GetHWND() const { return hwnd_; }

private:

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void CreatGWindow(const wchar_t* title, uint32_t windowSizeX, uint32_t windowSizeY);
	void ThrowAwayWindow();

private:

	WNDCLASS wc_;
	RECT wrc_;
	HWND hwnd_;


};



