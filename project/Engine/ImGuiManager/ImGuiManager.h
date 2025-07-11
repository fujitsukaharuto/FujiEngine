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
#include "externals/imgui/utilities/builders.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG



class MyWin;
class DXCom;

class ImGuiManager {
public:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	static ImGuiManager* GetInstance();
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(MyWin* myWin, DXCom* dxComon);

	/// <summary>
	/// 終了
	/// </summary>
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
	static void ImGuiDragDropButton(const char* buttonLabel, const char* payloadType, const std::function<void(const ImGuiPayload* payload)>& onDrop, const std::function<void(const ImGuiPayload* payload)>& onPreview);
	static void ImGuiDragButton(const char* label, const void* payloadData, size_t payloadSize, const char* payloadType);
#endif // _DEBUG


#ifdef _DEBUG

	void InitNodeTexture();

	ed::NodeId GenerateNodeId() { return ed::NodeId(nextId++); }
	ed::PinId GeneratePinId() { return ed::PinId(nextId++); }
	ed::LinkId GenerateLinkId() { return ed::LinkId(nextId++); }

	bool CanCreateLink(const Pin& a, const Pin& b);
	const MyNode* FindNodeByPinId(const ed::PinId& pinId, const std::vector<MyNode>& nodes);
	const Pin* FindPin(const ed::PinId& id, const std::vector<MyNode>& nodes);

	void HandleCreateLink(std::vector<Link>& links, const std::vector<MyNode>& nodes);
	void HandleDeleteLink(std::vector<Link>& links);
	void DrawNode(MyNode& node, ed::Utilities::BlueprintNodeBuilder& builder);
	void DrawNodeEditor(NodeGraph* nodeGraph);
	void DrawPinIcon(bool connected);

#endif // _DEBUG


private:
	DXCom* dxCommon_ = nullptr;
	uint32_t srvIndex;

	float winSizeX_ = 0.0f;
	float winSizeY_ = 0.0f;

#ifdef _DEBUG
	ImFont* font_japanese = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE backGroundHandle_;

	int nextId = 1;
	// ノードエディタのコンテキスト
	ax::NodeEditor::EditorContext* nodeEditorContext_ = nullptr;
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
