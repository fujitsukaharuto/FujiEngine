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
			// 拡張子がなければ追加
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

	// 保存完了後の通知
	if (showSuccessMessage) {
		SavedPopup(showSuccessMessage);
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
		LoadedPopup(showLoadSuccessMessage);
	}

	// エラーメッセージ（ファイルが存在しない）
	if (showLoadErrorMessage) {
		LoadErrorPopup(showLoadErrorMessage, fileName);
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

void JsonSerializer::ShowSaveEditorObjPopup(const EditorObj& obj) {
#ifdef _DEBUG
	// Save ボタンを押すとポップアップを開く
	if (ImGui::Button("Save EditorObj")) {
		ImGui::OpenPopup("Save EditorObj");
	}

	// ポップアップの中央配置
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static std::string fileName = obj.name;
	static bool showSuccessMessage = false;

	if (ImGui::BeginPopupModal("Save EditorObj", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file name to save the EditorObj:");
		// 一時バッファにコピーして表示・編集
		char buffer[128];
		strncpy_s(buffer, sizeof(buffer), fileName.c_str(), _TRUNCATE);

		// 編集
		if (ImGui::InputText("##filename", buffer, IM_ARRAYSIZE(buffer))) {
			fileName = buffer; // 編集結果を std::string に戻す
		}

		ImGui::Separator();

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			std::string path = fileName;
			if (path.empty()) {
				path = "default_EditorObj.json";
			}
			// 拡張子がなければ追加
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			SerializeEditorObj(obj, path);
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
#endif // _DEBUG
}

bool JsonSerializer::ShowLoadEditorObjPopup(EditorObj& obj) {
#ifdef _DEBUG
	bool result = false;
	// Load ボタン
	if (ImGui::Button("Load EditorObj")) {
		ImGui::OpenPopup("Load EditorObj");
	}

	// ポップアップ中央に配置
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static char fileName[128] = "default_EditorObj.json";
	static bool showLoadSuccessMessage = false;
	static bool showLoadErrorMessage = false;

	if (ImGui::BeginPopupModal("Load EditorObj", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file name to load the EditorObj:");
		ImGui::InputText("##load_filename", fileName, IM_ARRAYSIZE(fileName));

		ImGui::Separator();

		if (ImGui::Button("Load", ImVec2(120, 0))) {
			std::string path = fileName;
			if (path.empty()) {
				path = "default_EditorObj.json";
			}
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			if (DeserializeEditorObj(path, obj, true)) {
				showLoadSuccessMessage = true;
				result = true;
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
	return result;
#endif // _DEBUG
}

void JsonSerializer::SerializeEditorObj(const EditorObj& obj, const std::string& filePath) {
	json json;
	json["objectName"] = obj.name;
	json["modelName"] = obj.obj->GetModelName();
	// Vector3をそれぞれ配列として保存
	json["transform"]["translate"] = { obj.obj->transform.translate.x, obj.obj->transform.translate.y, obj.obj->transform.translate.z };
	json["transform"]["rotate"] = { obj.obj->transform.rotate.x, obj.obj->transform.rotate.y, obj.obj->transform.rotate.z };
	json["transform"]["scale"] = { obj.obj->transform.scale.x, obj.obj->transform.scale.y, obj.obj->transform.scale.z };

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

bool JsonSerializer::DeserializeEditorObj(const std::string& filePath, EditorObj& obj, bool isCreateCommand) {
	std::filesystem::path dir = "resource/Json";
	std::filesystem::path fullPath = dir / (filePath);
	std::ifstream ifs(fullPath);
	if (!ifs.is_open()) {
		return false;
	}

	json json;
	ifs >> json;
	ifs.close();


	// 名前・モデル名の読み取り
	if (json.contains("objectName")) {
		obj.name = json["objectName"].get<std::string>();
	}
	if (json.contains("modelName")) {
		obj.modelName = json["modelName"].get<std::string>();
	}

	Trans prevTransform = obj.obj->transform;
	// JSON配列からVector3を復元
	if (json.contains("transform")) {
		const auto& t = json["transform"];

		if (t.contains("translate")) {
			obj.obj->transform.translate.x = t["translate"][0];
			obj.obj->transform.translate.y = t["translate"][1];
			obj.obj->transform.translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			obj.obj->transform.rotate.x = t["rotate"][0];
			obj.obj->transform.rotate.y = t["rotate"][1];
			obj.obj->transform.rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			obj.obj->transform.scale.x = t["scale"][0];
			obj.obj->transform.scale.y = t["scale"][1];
			obj.obj->transform.scale.z = t["scale"][2];
		}
	}

	if (isCreateCommand) {
		CommandManager::TryCreatePropertyCommand(obj.obj->transform, prevTransform.translate, obj.obj->transform.translate, &Trans::translate);
		CommandManager::TryCreatePropertyCommand(obj.obj->transform, prevTransform.rotate, obj.obj->transform.rotate, &Trans::rotate);
		CommandManager::TryCreatePropertyCommand(obj.obj->transform, prevTransform.scale, obj.obj->transform.scale, &Trans::scale);
	}
	return true;
}

void JsonSerializer::SerializeJsonData([[maybe_unused]] const json& data, [[maybe_unused]]const std::string& filePath) {
	std::string fullPath = filePath;
	std::filesystem::path path(fullPath);
	if (path.extension() != ".json") {

		path.replace_extension(".json");
		fullPath = path.string();
	}
	std::ofstream file(fullPath);
	if (file.is_open()) {
		file << data.dump(4);
		file.close();
	}
}

json JsonSerializer::DeserializeJsonData(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return json();
	}
	json data;
	file >> data;
	file.close();
	return data;
}

void JsonSerializer::SavedPopup(bool& success) {
#ifdef _DEBUG
	if (success) {
		ImGui::OpenPopup("Saved!");
	}

	if (ImGui::BeginPopupModal("Saved!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("EditorObj saved successfully!");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			success = false;
		}
		ImGui::EndPopup();
	}
#endif // _DEBUG
}

void JsonSerializer::LoadedPopup(bool& success) {
#ifdef _DEBUG
	if (success) {
		ImGui::OpenPopup("Loaded!");
	}
	if (ImGui::BeginPopupModal("Loaded!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("EditorObj loaded successfully!");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			success = false;
		}
		ImGui::EndPopup();
	}
#endif // _DEBUG
}

void JsonSerializer::LoadErrorPopup(bool& error, const std::string& filePath) {
#ifdef _DEBUG
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
#endif // _DEBUG
}
