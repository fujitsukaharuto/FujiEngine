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

		float winSizeX_ = 0.0f;
		float winSizeY_ = 0.0f;

#ifdef _DEBUGMODE
		ImFont* font_japanese_ = nullptr;
#endif // _DEBUG
	private:
		ImGuiManager(const ImGuiManager&) = delete;
		const ImGuiManager& operator=(const ImGuiManager&) = delete;
	};


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