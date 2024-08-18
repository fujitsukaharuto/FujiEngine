#pragma once
#include <Windows.h>
#include <cstdint>

class MyWin
{
public:
	static const int kWindowWidth = 1280;
	static const int kWindowHeight = 720;

	static const wchar_t kWindowClassName[];

	static MyWin* GetInstance();
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	HWND GetHwnd() const;

public:

	void CreatGWindow(const wchar_t* title, int32_t clientWidth = kWindowWidth, int32_t clientHeight = kWindowHeight);
	void CreatWind(const wchar_t* name);
	bool ProcessMessage();
	void ThrowAwayWindow();

private:
	MyWin() = default;
	~MyWin() = default;
	MyWin(const MyWin&) = delete;
	const MyWin& operator=(const MyWin&) = delete;
private:
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};
	RECT wrc_;

};
