#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_node_editor.h"
#include "NodeGraph.h"
#include "utilities/builders.h"
#include "utilities/widgets.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG


namespace Core {
	class MyWin;
}
class DXCom;

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

		void Initialize(MyWin* myWin, DXCom* dxComon);
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
#ifdef _DEBUG
	/// <summary>Drag＆Dropができるボタン</summary>
		static void ImGuiDragDropButton(const char* buttonLabel, const char* payloadType, const std::function<void(const ImGuiPayload* payload)>& onDrop, const std::function<void(const ImGuiPayload* payload)>& onPreview);
		/// <summary>Dragができるボタン</summary>
		static void ImGuiDragButton(const char* label, const void* payloadData, size_t payloadSize, const char* payloadType);

		/// <summary>ポップアップを開くボタン</summary>
		static void ImGuiPopUpButton(const char* buttonLabel, const char* popupName, std::function<void()> contentFunc);
#endif // _DEBUG


#ifdef _DEBUG

		/// <summary>Nodeに使用するTextureの初期化</summary>
		void InitNodeTexture();

		/// <summary>NodeIDの生成</summary>
		ed::NodeId GenerateNodeId() { return ed::NodeId(nextId++); }
		/// <summary>PinIDの生成</summary>
		ed::PinId GeneratePinId() { return ed::PinId(nextId++); }
		/// <summary>LinkIDの生成</summary>
		ed::LinkId GenerateLinkId() { return ed::LinkId(nextId++); }

		/// <summary>Linkできるのかどうかのチェック</summary>
		bool CanCreateLink(const Pin& a, const Pin& b);
		/// <summary>PinIDからノードを探す</summary>
		const MyNode* FindNodeByPinId(const ed::PinId& pinId, const std::vector<MyNode>& nodes);
		/// <summary>Pinを探す</summary>
		const Pin* FindPin(const ed::PinId& id, const std::vector<MyNode>& nodes);

		/// <summary>Linkを結ぶのか確認</summary>
		void HandleCreateLink(std::vector<Link>& links, const std::vector<MyNode>& nodes);
		/// <summary>Linkを削除するのか確認</summary>
		void HandleDeleteLink(std::vector<Link>& links);
		/// <summary>Linkを結ぶ処理</summary>
		void CreateLink(std::vector<Link>& links, const Pin& input, const Pin& output);
		/// <summary>Nodeの描画</summary>
		void DrawNode(MyNode& node, ed::Utilities::BlueprintNodeBuilder& builder);
		/// <summary>NodeEditorの描画</summary>
		void DrawNodeEditor(NodeGraph* nodeGraph);
		/// <summary>Pinの描画</summary>
		void DrawPinIcon(bool connected, ax::Widgets::IconType icon = ax::Widgets::IconType::Circle);

#endif // _DEBUG
	private:

		void SetupModernStyle();

#ifdef _DEBUG

		/// <summary>Textureのセレクトメニューの表示</summary>
		void TextureSelectMenu(NodeGraph* nodeGraph);

#endif // _DEBUG


	private:
		DXCom* dxCommon_ = nullptr;
		uint32_t srvIndex;

		float winSizeX_ = 0.0f;
		float winSizeY_ = 0.0f;

#ifdef _DEBUG
		ImFont* font_japanese_ = nullptr;

		D3D12_GPU_DESCRIPTOR_HANDLE backGroundHandle_;

		int nextId = 1;
		// ノードエディタのコンテキスト
		ax::NodeEditor::EditorContext* nodeEditorContext_ = nullptr;
		ed::NodeId selectNodeID_;
#endif // _DEBUG
	private:
		ImGuiManager(const ImGuiManager&) = delete;
		const ImGuiManager& operator=(const ImGuiManager&) = delete;
	};


	struct ParticleGroupSelector {
#ifdef _DEBUG
		std::vector<std::string> items[2];  // 0: false, 1: true
		std::string selected[2];

		void Show(std::function<void(const std::string&, bool)> on_move);
#endif // _DEBUG
	};
}