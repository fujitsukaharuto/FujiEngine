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

		/// <summary>
		/// ゲーム画面の映し先
		/// </summary>
		struct GameView {
			float x, y, width, height;	// クライアント座標の矩形
			bool isOnPanel;				// デバッグGUIのパネルに映しているか。false ならバックバッファへ直に描く
			bool isHovered;				// パネル上の画像にマウスが乗っているか。パネルの上では WantCaptureMouse が常に立つ
		};

		/// <summary>画面全体へ直に描く状態へ戻す</summary>
		static void ResetGameView();

		/// <summary>与えた領域にアスペクト比を保って収まるよう、パネル上のゲーム画面の矩形を決める</summary>
		static void FitGameView(float x, float y, float width, float height);

		static void SetGameViewHovered(bool isHovered);

		/// <summary>ゲーム画面の映し先。マウス座標の変換に使う</summary>
		static const GameView& GetGameView();

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