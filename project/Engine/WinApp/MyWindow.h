#pragma once
#include <Windows.h>
#include <cstdint>

class MyWin {
public:
	MyWin() = default;
	~MyWin() = default;

public:
	static const int kWindowWidth = 1280;
	static const int kWindowHeight = 720;

public:

	static const wchar_t kWindowClassName[];

	static MyWin* GetInstance();

	void Initialize();

	void Finalize();

	bool ProcessMessage();


	const HWND& GetHwnd() const { return hwnd_; }
	const WNDCLASS& GetWNDCLASS() const { return wc_; }
	const RECT& GetRECT() const { return wrc_; }



private:

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void CreateGWindow(const wchar_t* name, uint32_t windowSizeX, uint32_t windowSizeY);

	void ThrowAwayWindow();


private:
	MyWin(const MyWin&) = delete;
	const MyWin& operator=(const MyWin&) = delete;
private:

	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};
	RECT wrc_;

};
