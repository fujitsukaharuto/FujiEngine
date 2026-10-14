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
		// ゲームを描く解像度。ウィンドウの大きさとは別で、ウィンドウを広げても変わらない
		// (最後にウィンドウへ拡大して映す)
		static const int kWindowWidth = 1280;
		static const int kWindowHeight = 720;

	public:

		/// <summary>
		/// ゲーム画面の映し先
		/// </summary>
		struct GameView {
			float x, y, width, height;	// クライアント座標の矩形
			bool isOnPanel;				// デバッグGUIのパネルに映しているか。false ならウィンドウいっぱい(比率は保つ)へ直に描く
			bool isHovered;				// パネル上の画像にマウスが乗っているか。パネルの上では WantCaptureMouse が常に立つ
		};

		/// <summary>ウィンドウへ直に描く状態へ戻す。比率を保って中央に収め、余りは黒帯になる</summary>
		static void ResetGameView();

		/// <summary>与えた領域にアスペクト比を保って収まるよう、パネル上のゲーム画面の矩形を決める</summary>
		static void FitGameView(float x, float y, float width, float height);

		static void SetGameViewHovered(bool isHovered);
		/// <summary>エディタがゲーム画面上のマウスを使っている(スプライトのドラッグ等)</summary>
		/// <remarks>立っている間はオブジェクトのピックをしない</remarks>
		static void SetGameViewCaptured(bool isCaptured);
		static bool IsGameViewCaptured();

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

		/// <summary>枠なしのフルスクリーンと通常のウィンドウを切り替える。F11 と Alt+Enter でも切り替わる</summary>
		/// <remarks>ウィンドウを今いるモニターいっぱいに広げるだけで、画面モードは変えない</remarks>
		void SetFullscreen(bool isFullscreen);
		void ToggleFullscreen() { SetFullscreen(!isFullscreen_); }
		bool IsFullscreen() const { return isFullscreen_; }


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

		bool isFullscreen_ = false;
		// フルスクリーンから戻すときの位置と大きさ
		WINDOWPLACEMENT windowedPlacement_{ sizeof(WINDOWPLACEMENT) };
	};
}