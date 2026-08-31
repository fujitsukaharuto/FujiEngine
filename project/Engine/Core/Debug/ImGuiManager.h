#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#ifdef _DEBUGMODE
#include "imgui.h"
#endif // _DEBUG


namespace Core {
	class MyWin;
}
namespace DXC { class DXCom; }

namespace Core {
	/// <summary>
	/// ImGui関連の管理クラス
	/// </summary>
	class ImGuiManager {
	public:
		ImGuiManager() = default;
		~ImGuiManager() = default;

		static ImGuiManager* GetInstance();
	public:

		void Initialize(MyWin* myWin, DXC::DXCom* dxComon);
		void Fin();

		/// <summary>
		/// ImGui受付
		/// </summary>
		void Begin();

		/// <summary>
		/// ImGui締切
		/// </summary>
		void End();

		/// <summary>
		/// 描画
		/// </summary>
		void Draw();

		/// <summary>
		/// 日本語フォントを使うためにセットする
		/// </summary>
		void SetFontJapanese();

		/// <summary>
		/// 英語用のフォントに戻す
		/// </summary>
		void UnSetFont();

		///---------------------------------------------------------------------------------------
		/// ImGuiUtility
		///---------------------------------------------------------------------------------------
#ifdef _DEBUGMODE
	/// <summary>Drag＆Dropができるボタン</summary>
		static void ImGuiDragDropButton(const char* buttonLabel, const char* payloadType, const std::function<void(const ImGuiPayload* payload)>& onDrop, const std::function<void(const ImGuiPayload* payload)>& onPreview);
		/// <summary>Dragができるボタン</summary>
		static void ImGuiDragButton(const char* label, const void* payloadData, size_t payloadSize, const char* payloadType);

		/// <summary>ポップアップを開くボタン</summary>
		static void ImGuiPopUpButton(const char* buttonLabel, const char* popupName, const std::function<void()>& contentFunc);
#endif // _DEBUG


	private:

		void SetupModernStyle();

	private:
		DXC::DXCom* dxCommon_ = nullptr;
		uint32_t srvIndex;

#ifdef _DEBUGMODE
		ImFont* font_japanese_ = nullptr;
#endif // _DEBUG
	private:
		ImGuiManager(const ImGuiManager&) = delete;
		const ImGuiManager& operator=(const ImGuiManager&) = delete;
	};


#ifdef _DEBUGMODE
	// TextColoredなどスタイル外の色は SetupModernStyle のリニア化を通らないので、変換済みの色をここから配る
	/// <summary>正常・対応済みを示す文字色</summary>
	ImVec4 ImGuiTextOk();
	/// <summary>注意を促す文字色</summary>
	ImVec4 ImGuiTextWarn();
	/// <summary>失敗・非対応を示す文字色</summary>
	ImVec4 ImGuiTextError();

	/// <summary>デバッグウィンドウの開閉状態。メインメニューの View から切り替える</summary>
	class DebugWindows {
	public:
		/// <summary>名前ごとの表示フラグ。初めて渡した名前はその場で登録される</summary>
		static bool* Visible(const char* name);
		/// <summary>登録済みのウィンドウをメニュー項目として並べる</summary>
		static void MenuItems();
	};

	/// <summary>View メニューで開閉できるデバッグウィンドウ</summary>
	/// <remarks>if (DebugWindow w{"Camera"}) { 中身 } と書く。閉じている・畳まれている間は中身を呼ばず、スコープを抜けると End まで済ませる</remarks>
	class DebugWindow {
	public:
		explicit DebugWindow(const char* name, ImGuiWindowFlags flags = 0);
		~DebugWindow();

		DebugWindow(const DebugWindow&) = delete;
		DebugWindow& operator=(const DebugWindow&) = delete;

		explicit operator bool() const { return isDrawable_; }

	private:
		bool isBegun_ = false;
		bool isDrawable_ = false;
	};
#endif // _DEBUGMODE

	/// <summary>
	/// パーティクルグループを2つのリスト間で移動させるGUI
	/// </summary>
	struct ParticleGroupSelector {
#ifdef _DEBUGMODE
		std::vector<std::string> items[2];  // 0: false, 1: true
		std::string selected[2];

		void Show(const std::function<void(const std::string&, bool)>& on_move);
#endif // _DEBUG
	};
}