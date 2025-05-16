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
	void Init(MyWin* myWin, DXCom* dxComon);

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

	void DrawNodeEditor();

private:
	DXCom* dxCommon_ = nullptr;
	uint32_t srvIndex;

	float winSizeX_ = 0.0f;
	float winSizeY_ = 0.0f;

#ifdef _DEBUG
	ImFont* font_japanese = nullptr;

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
