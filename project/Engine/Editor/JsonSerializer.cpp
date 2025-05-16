#include "JsonSerializer.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/CommandManager.h"
#include <fstream>
#include <iostream>

void JsonSerializer::ShowSaveTransformPopup(const Trans& transform) {
#ifdef _DEBUG
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
			// 拡張子がなければ追加（任意）
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			SerializeTransform(transform, path);
			showSuccessMessage = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// 保存完了後の通知（任意）
	if (showSuccessMessage) {
		ImGui::OpenPopup("Saved!");
	}

	if (ImGui::BeginPopupModal("Saved!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Transform saved successfully!");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			showSuccessMessage = false;
		}
		ImGui::EndPopup();
	}
#endif // _DEBUG
}

void JsonSerializer::ShowLoadTransformPopup(Trans& transform) {
#ifdef _DEBUG
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

			if (DeserializeTransform(path, transform, true)) {
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
		ImGui::OpenPopup("Loaded!");
	}
	if (ImGui::BeginPopupModal("Loaded!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Transform loaded successfully!");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			showLoadSuccessMessage = false;
		}
		ImGui::EndPopup();
	}

	// エラーメッセージ（ファイルが存在しない）
	if (showLoadErrorMessage) {
		ImGui::OpenPopup("Load Error");
	}
	if (ImGui::BeginPopupModal("Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("File not found:\n%s", fileName);
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			showLoadErrorMessage = false;
		}
		ImGui::EndPopup();
	}
#endif // _DEBUG
}

void JsonSerializer::SerializeTransform(const Trans& transform, const std::string& filePath) {
	json json;
	// Vector3をそれぞれ配列として保存
	json["translate"] = { transform.translate.x, transform.translate.y, transform.translate.z };
	json["rotate"] = { transform.rotate.x, transform.rotate.y, transform.rotate.z };
	json["scale"] = { transform.scale.x, transform.scale.y, transform.scale.z };

	std::filesystem::path dir = "resource/Json";
	std::filesystem::create_directories(dir); // ディレクトリが無ければ作成
	std::filesystem::path fullPath = dir / (filePath);

	// 書き込み
	std::ofstream ofs(fullPath);
	if (ofs.is_open()) {
		ofs << json.dump(4); // インデント付きで出力
		ofs.close();
	}
}

bool JsonSerializer::DeserializeTransform(const std::string& filePath, Trans& outTransform, bool isCreateCommand) {
	std::filesystem::path dir = "resource/Json";
	std::filesystem::path fullPath = dir / (filePath);
	std::ifstream ifs(fullPath);
	if (!ifs.is_open()) {
		return false;
	}

	json json;
	ifs >> json;
	ifs.close();

	Trans prevTransform = outTransform;
	// JSON配列からVector3を復元
	if (json.contains("translate") && json["translate"].is_array()) {
		outTransform.translate.x = json["translate"][0];
		outTransform.translate.y = json["translate"][1];
		outTransform.translate.z = json["translate"][2];
	}
	if (json.contains("rotate") && json["rotate"].is_array()) {
		outTransform.rotate.x = json["rotate"][0];
		outTransform.rotate.y = json["rotate"][1];
		outTransform.rotate.z = json["rotate"][2];
	}
	if (json.contains("scale") && json["scale"].is_array()) {
		outTransform.scale.x = json["scale"][0];
		outTransform.scale.y = json["scale"][1];
		outTransform.scale.z = json["scale"][2];
	}

	if (isCreateCommand) {
		CommandManager::TryCreatePropertyCommand(outTransform, prevTransform.translate, outTransform.translate, &Trans::translate);
		CommandManager::TryCreatePropertyCommand(outTransform, prevTransform.rotate, outTransform.rotate, &Trans::rotate);
		CommandManager::TryCreatePropertyCommand(outTransform, prevTransform.scale, outTransform.scale, &Trans::scale);
	}
	return true;
}
