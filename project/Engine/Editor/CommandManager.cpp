#include "CommandManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Editor/JsonSerializer.h"
#include <fstream>

CommandManager::~CommandManager() {
}

void CommandManager::Execute(std::unique_ptr<ICommand> command) {
	command->Do();
	undoStack.push(std::move(command));
	// Redoスタックは新しい操作を行った時点で無効になる
	while (!redoStack.empty()) redoStack.pop();
}

void CommandManager::Undo() {
	if (undoStack.empty()) return;

	auto command = std::move(undoStack.top());
	undoStack.pop();
	command->UnDo();
	redoStack.push(std::move(command));
}

void CommandManager::Redo() {
	if (redoStack.empty()) return;

	auto command = std::move(redoStack.top());
	redoStack.pop();
	command->ReDo();
	undoStack.push(std::move(command));
}

void CommandManager::CheckInputForUndoRedo() {
	if (Input::GetInstance()->PushKey(DIK_LCONTROL) && Input::GetInstance()->TriggerKey(DIK_Z)) {
		if (CanUndo()) {
			Undo();
		}
	} else if (Input::GetInstance()->PushKey(DIK_LCONTROL) && Input::GetInstance()->TriggerKey(DIK_Y)) {
		if (CanRedo()) {
			Redo();
		}
	}
}

void CommandManager::Reset() {
	undoStack = std::stack<std::unique_ptr<ICommand>>();
	redoStack = std::stack<std::unique_ptr<ICommand>>();

	objectList.clear();
	headerNames.clear();
	nameHashes.clear();
}

void CommandManager::Finalize() {
	undoStack = std::stack<std::unique_ptr<ICommand>>();
	redoStack = std::stack<std::unique_ptr<ICommand>>();

	loadObj.reset();

	objectList.clear();
	headerNames.clear();
	nameHashes.clear();
}

void CommandManager::Draw() {
#ifdef _DEBUG
	for (auto& group : objectList) {
		EditorObj* obj = group.second.get();
		if (obj->isActive) {
			obj->obj->Draw();
		}
	}
#endif // _DEBUG
}

void CommandManager::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	for (auto& group: objectList) {
		EditorObj* obj = group.second.get();

		// 表示名（キャッシュが無ければ初期化）
		if (headerNames.find(group.first) == headerNames.end()) {
			std::string defaultName = obj->name.empty()
				? "EditorObj" + std::to_string(group.first)
				: obj->name;

			headerNames[group.first] = defaultName + "##" + std::to_string(group.first);
			nameHashes[group.first] = std::hash<std::string>{}(defaultName);
		}

		if (obj->isActive) {
			// CollapsingHeader 表示
			if (ImGui::CollapsingHeader(headerNames[group.first].c_str())) {
				// 編集用のラベル（ImGui ID用）
				std::string inputLabel = "##input" + std::to_string(group.first);

				// ImGuiの入力欄で名前編集（最大64文字くらい推奨）
				static char nameBuffer[64]; // 固定長バッファ（使い回し可）
				strncpy_s(nameBuffer, sizeof(nameBuffer), obj->name.c_str(), _TRUNCATE);

				ImGui::Text("Name"); ImGui::SameLine();
				if (ImGui::InputText(inputLabel.c_str(), nameBuffer, sizeof(nameBuffer),
					ImGuiInputTextFlags_EnterReturnsTrue)) {
					// Enterで確定された場合のみ name を更新
					obj->name = std::string(nameBuffer);

					// ラベル再生成（空の場合はデフォルト）
					std::string newName = obj->name.empty()
						? "EditorObj" + std::to_string(group.first)
						: obj->name;

					headerNames[group.first] = newName + "##" + std::to_string(group.first);
					nameHashes[group.first] = std::hash<std::string>{}(newName);
				}

				obj->obj->DebugGUI();

				JsonSerializer::ShowSaveEditorObjPopup(*obj);

				std::string deleteButtonLabel = "Delete##" + std::to_string(group.first);
				if (ImGui::Button(deleteButtonLabel.c_str())) {
					auto deleteCommand = std::make_unique<DeleteObjCommand>(group.first);
					Execute(std::move(deleteCommand));
					continue;
				}
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Add New Object");

	// 静的入力バッファ（入力欄）
	static char newObjName[64] = "NewObject";

	ImGui::InputText("Obj Name", newObjName, sizeof(newObjName));

	// 追加ボタンが押されたら
	if (ImGui::Button("Add")) {
		// 新しいオブジェクト名を決定（空文字ならデフォルト）
		int newId = CommandManager::GetInstance()->nextObjId++;
		std::string nameStr = newObjName[0] == '\0'
			? "EditorObj" + std::to_string(newId)
			: std::string(newObjName);

		// コマンド生成＆実行
		auto command = std::make_unique<CreateObjCommand>(newId, nameStr);
		Execute(std::move(command));

		// 入力欄クリア
		std::memset(newObjName, 0, sizeof(newObjName));
	}

	if (!loadObj) {
		loadObj = std::make_unique<EditorObj>();
		loadObj->obj = std::make_unique<Object3d>();
		loadObj->obj->CreateSphere();
	}
	if (JsonSerializer::ShowLoadEditorObjPopup(*loadObj)) {
		int newId = CommandManager::GetInstance()->nextObjId++;
		auto command = std::make_unique<CreateObjCommand>(newId, loadObj->name, loadObj->modelName);
		Execute(std::move(command));
		objectList[newId]->obj->transform = loadObj->obj->transform;
	}
	ImGui::Separator();
	ImGui::Text("All EditorObjects");
	SaveAllEditorOBJ(); ImGui::SameLine();
	LoadAllEditorOBJ();

	GarbageCollect();
	ImGui::Unindent();
#endif // DEBUG
}

std::shared_ptr<EditorObj> CommandManager::GetEditorObject(int id) const {
	auto it = objectList.find(id);
	return (it != objectList.end()) ? it->second : nullptr;
}

void CommandManager::GarbageCollect() {
	// 非アクティブなオブジェクトを削除
	std::erase_if(objectList, [](const auto& pair) {
		return !pair.second->isActive;
		});

	// ヘッダー名やハッシュも削除しておく
	for (auto it = headerNames.begin(); it != headerNames.end(); ) {
		if (objectList.find(it->first) == objectList.end()) {
			it = headerNames.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = nameHashes.begin(); it != nameHashes.end(); ) {
		if (objectList.find(it->first) == objectList.end()) {
			it = nameHashes.erase(it);
		} else {
			++it;
		}
	}
}

void CommandManager::EditorOBJSave(const std::string& filePath) {
	nlohmann::json jsonList = nlohmann::json::array();  // 配列として初期化

	for (auto& group : objectList) {
		EditorObj* obj = group.second.get();
		nlohmann::json json;  // 各オブジェクトごとの JSON を作成

		json["objectName"] = obj->name;
		json["modelName"] = obj->obj->GetModelName();

		// Vector3を配列として保存
		json["transform"]["translate"] = {
			obj->obj->transform.translate.x,
			obj->obj->transform.translate.y,
			obj->obj->transform.translate.z
		};
		json["transform"]["rotate"] = {
			obj->obj->transform.rotate.x,
			obj->obj->transform.rotate.y,
			obj->obj->transform.rotate.z
		};
		json["transform"]["scale"] = {
			obj->obj->transform.scale.x,
			obj->obj->transform.scale.y,
			obj->obj->transform.scale.z
		};

		jsonList.push_back(json);  // 配列に追加
	}

	// 最終的なJSON出力
	nlohmann::json finalJson;
	finalJson["objects"] = jsonList;

	std::filesystem::path dir = "resource/Json";
	std::filesystem::create_directories(dir); // ディレクトリが無ければ作成
	std::filesystem::path fullPath = dir / (filePath);

	// 書き込み
	std::ofstream ofs(fullPath);
	if (ofs.is_open()) {
		ofs << finalJson.dump(4); // インデント付きで出力
		ofs.close();
	}
}

void CommandManager::SaveAllEditorOBJ() {
#ifdef _DEBUG
	// Save ボタンを押すとポップアップを開く
	if (ImGui::Button("Save AllEditorObj")) {
		ImGui::OpenPopup("Save AllEditorObj");
	}

	// ポップアップの中央配置
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static std::string fileName = "all_EditorObj";
	static bool showSuccessMessage = false;

	if (ImGui::BeginPopupModal("Save AllEditorObj", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
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
				path = "all_EditorObj.json";
			}
			// 拡張子がなければ追加
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			EditorOBJSave(path);
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
		JsonSerializer::SavedPopup(showSuccessMessage);
	}
#endif // _DEBUG
}

bool CommandManager::EditorOBJLoad(const std::string& filePath) {
	std::filesystem::path fullPath = std::filesystem::path("resource/Json") / filePath;
	// ファイルが存在しない場合は false を返す
	if (!std::filesystem::exists(fullPath)) {
		return false;
	}
	std::ifstream ifs(fullPath);
	if (!ifs.is_open()) {
		return false;
	}
	nlohmann::json json;
	try {
		ifs >> json;
	}
	catch (...) {
		return false;
	}
	ifs.close();
	if (!json.contains("objects") || !json["objects"].is_array()) {
		return false;
	}

	for (const auto& objJson : json["objects"]) {
		// 基本情報の読み込み
		std::string name = objJson.value("objectName", "Unnamed");
		std::string modelName = objJson.value("modelName", "DefaultModel");

		// 新しいオブジェクトIDの取得
		int newId = CommandManager::GetInstance()->nextObjId++;

		// 作成コマンドの生成と実行
		auto command = std::make_unique<CreateObjCommand>(newId, name, modelName);
		Execute(std::move(command));

		// トランスフォームの反映
		auto& obj = objectList[newId]->obj;

		if (objJson.contains("transform")) {
			const auto& t = objJson["transform"];

			if (t.contains("translate")) {
				obj->transform.translate.x = t["translate"][0];
				obj->transform.translate.y = t["translate"][1];
				obj->transform.translate.z = t["translate"][2];
			}
			if (t.contains("rotate")) {
				obj->transform.rotate.x = t["rotate"][0];
				obj->transform.rotate.y = t["rotate"][1];
				obj->transform.rotate.z = t["rotate"][2];
			}
			if (t.contains("scale")) {
				obj->transform.scale.x = t["scale"][0];
				obj->transform.scale.y = t["scale"][1];
				obj->transform.scale.z = t["scale"][2];
			}
		}
	}

	return true; // 正常に読み込んだ
}

void CommandManager::LoadAllEditorOBJ() {
#ifdef _DEBUG
	// Load ボタン
	if (ImGui::Button("Load AllEditorObj")) {
		ImGui::OpenPopup("Load AllEditorObj");
	}

	// ポップアップ中央に配置
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	static char fileName[128] = "all_EditorObj.json";
	static bool showLoadSuccessMessage = false;
	static bool showLoadErrorMessage = false;

	if (ImGui::BeginPopupModal("Load AllEditorObj", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file name to load the EditorObj:");
		ImGui::InputText("##load_filename", fileName, IM_ARRAYSIZE(fileName));

		ImGui::Separator();

		if (ImGui::Button("Load", ImVec2(120, 0))) {
			std::string path = fileName;
			if (path.empty()) {
				path = "all_EditorObj.json";
			}
			if (path.find('.') == std::string::npos) {
				path += ".json";
			}

			if (EditorOBJLoad(path)) {
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
		JsonSerializer::LoadedPopup(showLoadSuccessMessage);
	}

	// エラーメッセージ（ファイルが存在しない）
	if (showLoadErrorMessage) {
		JsonSerializer::LoadErrorPopup(showLoadErrorMessage, fileName);
	}
#endif // _DEBUG
}
