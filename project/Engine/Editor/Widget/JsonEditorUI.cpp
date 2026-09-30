#include "JsonEditorUI.h"
#ifdef _DEBUGMODE
#include <fstream>
#include <filesystem>
#include <json.hpp>

#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Editor/Command/CommandManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"

using namespace Core;
using namespace Math;
using namespace Editor;


namespace {
	constexpr int JSON_INDENT_WIDTH = 4;
}


void JsonEditorUI::ShowSaveTransformPopup(const Trans& transform) {
	// Save ボタンを押すとポップアップを開く
	if (ImGui::Button("Save Transform")) {
		ImGui::OpenPopup("Save Transform");
	}

	// ポップアップの中央配置
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static char fileName[128] = "default_transform.json";
	static bool showSuccessMessage = false;

	if (ImGui::BeginPopupModal("Save Transform", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file name to save the transform:");
		ImGui::InputText("##filename", fileName, IM_ARRAYSIZE(fileName));

		ImGui::Separator();

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			std::string path = fileName;
			if (path.empty()) {
				path = "default_transform.json";
			}
			// 拡張子がなければ追加
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			JsonSerializer::SerializeTransform(transform, path);
			showSuccessMessage = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// 保存完了後の通知
	if (showSuccessMessage) {
		SavedPopup(showSuccessMessage);
	}
}

void JsonEditorUI::ShowLoadTransformPopup(Trans& transform) {
	// Load ボタン
	if (ImGui::Button("Load Transform")) {
		ImGui::OpenPopup("Load Transform");
	}

	// ポップアップ中央に配置
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static char fileName[128] = "default_transform.json";
	static bool showLoadSuccessMessage = false;
	static bool showLoadErrorMessage = false;

	if (ImGui::BeginPopupModal("Load Transform", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file name to load the transform:");
		ImGui::InputText("##load_filename", fileName, IM_ARRAYSIZE(fileName));

		ImGui::Separator();

		if (ImGui::Button("Load", ImVec2(120, 0))) {
			std::string path = fileName;
			if (path.empty()) {
				path = "default_transform.json";
			}
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			Trans prevTransform = transform;
			if (JsonSerializer::DeserializeTransform(path, transform)) {
				// 読み込みで動いた分をUndoできるようにする
				CommandManager::TryCreatePropertyCommand(transform, prevTransform.translate, transform.translate, &Trans::translate);
				CommandManager::TryCreatePropertyCommand(transform, prevTransform.rotate, transform.rotate, &Trans::rotate);
				CommandManager::TryCreatePropertyCommand(transform, prevTransform.scale, transform.scale, &Trans::scale);
				showLoadSuccessMessage = true;
			} else {
				showLoadErrorMessage = true;
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (showLoadSuccessMessage) {
		LoadedPopup(showLoadSuccessMessage);
	}

	// エラーメッセージ（ファイルが存在しない）
	if (showLoadErrorMessage) {
		LoadErrorPopup(showLoadErrorMessage, fileName);
	}
}

void JsonEditorUI::SavedPopup(bool& success) {
	if (success) {
		ImGui::OpenPopup("Saved!");
	}

	if (ImGui::BeginPopupModal("Saved!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Saved successfully!");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			success = false;
		}
		ImGui::EndPopup();
	}
}

void JsonEditorUI::LoadedPopup(bool& success) {
	if (success) {
		ImGui::OpenPopup("Loaded!");
	}
	if (ImGui::BeginPopupModal("Loaded!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Loaded successfully!");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			success = false;
		}
		ImGui::EndPopup();
	}
}

void JsonEditorUI::LoadErrorPopup(bool& error, const std::string& filePath) {
	if (error) {
		ImGui::OpenPopup("Load Error");
	}
	if (ImGui::BeginPopupModal("Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("File not found:\n%s", filePath.c_str());
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			error = false;
		}
		ImGui::EndPopup();
	}
}
#endif // _DEBUGMODE
