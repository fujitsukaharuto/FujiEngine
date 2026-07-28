#pragma once
#include <Windows.h>
#include <cstdint>

namespace Core {
	/// <summary>
	/// Windowsアプリケーションのウィンドウ管理のクラス
	/// </summary>
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

		/// <summary>
		/// メッセージを処理する
		/// </summary>
		/// <returns>bool</returns>
		bool ProcessMessage();


		//========================================================================*/
		//* Getter
		const HWND& GetHwnd() const { return hwnd_; }
		const WNDCLASS& GetWNDCLASS() const { return wc_; }
		const RECT& GetRECT() const { return wrc_; }
		int GetClientWidth()const { return clientWidth_; };
		int GetClientHeight()const { return clientHeight_; };

	private:

		/// <summary>ウィンドウプロシージャ関数</summary>
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		/// <summary>指定された名前とサイズでゲーム用ウィンドウを生成</summary>
		void CreateGWindow(const wchar_t* name, uint32_t windowSizeX, uint32_t windowSizeY);

		/// <summary>ウィンドウを破棄し、関連リソースを解放</summary>
		void ThrowAwayWindow();


	private:
		MyWin(const MyWin&) = delete;
		const MyWin& operator=(const MyWin&) = delete;
	private:

		HWND hwnd_ = nullptr;
		WNDCLASS wc_{};
		RECT wrc_;

		int clientWidth_ = kWindowWidth;
		int clientHeight_ = kWindowHeight;
	};
}