#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

class MyWin;
class DXCom;

class ImGuiManager {
public:
	ImGuiManager() = default;
	~ImGuiManager() = default;
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

private:
	DXCom* dxCommon_ = nullptr;
	uint32_t srvIndex;

	float winSizeX_ = 0.0f;
	float winSizeY_ = 0.0f;

#ifdef _DEBUG
	ImFont* font_japanese = nullptr;
#endif // _DEBUG
private:
	ImGuiManager(const ImGuiManager&) = delete;
	const ImGuiManager& operator=(const ImGuiManager&) = delete;
};


struct ParticleGroupSelector {
#ifdef _DEBUG
	std::vector<std::string> items[2];  // 0: false, 1: true
	std::string selected[2];

	void Show(std::function<void(const std::string&, bool)> on_move) {
		const float listBoxHeight = 200.0f;

		if (ImGui::BeginTable("ParticleGroupTable", 3, ImGuiTableFlags_None)) {
			ImGui::TableSetupColumn("Emitte: False", ImGuiTableColumnFlags_WidthStretch);    // Left side
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);      // Buttons
			ImGui::TableSetupColumn("Emitte: True", ImGuiTableColumnFlags_WidthStretch);    // Right side
			ImGui::TableNextRow();

			ImGui::TableHeadersRow();
			ImGui::TableNextRow();

			// --- 左列（false） ---
			ImGui::TableSetColumnIndex(0);
			if (ImGui::BeginListBox("##falseList", ImVec2(-FLT_MIN, listBoxHeight))) {
				for (const auto& name : items[0]) {
					bool is_selected = (selected[0] == name);
					if (ImGui::Selectable(name.c_str(), is_selected)) {
						selected[0] = name;
					}
				}
				ImGui::EndListBox();
			}

			// --- 中央列（ボタン） ---
			ImGui::TableSetColumnIndex(1);
			{
				//// 中央揃えするためにダミースペースを使う
				//float buttonHeight = ImGui::GetFrameHeight();
				//float spacing = (listBoxHeight - buttonHeight * 2.0f - ImGui::GetStyle().ItemSpacing.y) * 0.5f;
				//ImGui::Dummy(ImVec2(0.0f, spacing));

				if (ImGui::Button(">")) {
					if (!selected[0].empty()) {
						items[1].push_back(selected[0]);
						items[0].erase(std::remove(items[0].begin(), items[0].end(), selected[0]), items[0].end());
						on_move(selected[0], true);
						selected[0].clear();
					}
				}

				if (ImGui::Button("<")) {
					if (!selected[1].empty()) {
						items[0].push_back(selected[1]);
						items[1].erase(std::remove(items[1].begin(), items[1].end(), selected[1]), items[1].end());
						on_move(selected[1], false);
						selected[1].clear();
					}
				}
			}

			// --- 右列（true） ---
			ImGui::TableSetColumnIndex(2);
			if (ImGui::BeginListBox("##trueList", ImVec2(-FLT_MIN, listBoxHeight))) {
				for (const auto& name : items[1]) {
					bool is_selected = (selected[1] == name);
					if (ImGui::Selectable(name.c_str(), is_selected)) {
						selected[1] = name;
					}
				}
				ImGui::EndListBox();
			}

			ImGui::EndTable();
		}
	}
#endif // _DEBUG
};
