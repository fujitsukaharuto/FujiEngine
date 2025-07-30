#include "CommandManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Model/ModelManager.h"
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

void CommandManager::StackReset() {
	undoStack = std::stack<std::unique_ptr<ICommand>>();
	redoStack = std::stack<std::unique_ptr<ICommand>>();
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
		if (obj) {
			if (obj->isActive) {
				obj->obj->Draw();
			}
		}
	}
#endif // _DEBUG
}

void CommandManager::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	for (auto& group: objectList) {
		EditorObj* obj = group.second.get();
		if (obj) {
			if (ModelManager::GetInstance()->GetPickedID() == obj->obj->GetObjID() && ModelManager::GetInstance()->GetIsPicked()) {
				openedHeaderID = obj->id;
			}
			if (ModelManager::GetInstance()->GetPickedID() == obj->obj->GetObjID() && ModelManager::GetInstance()->GetIsOnce()) {
				openedHeaderID = obj->id;
			}
			if (obj->parent == -1) {
				EditorObjGUI(*obj);
			} else if (!objectList[obj->parent]) {
				if (obj->obj->IsHaveParent()) {
					obj->obj->SetParent(nullptr);
				}
				EditorObjGUI(*obj);
			}
		}
	}

	ImGui::SeparatorText("Add New Object");
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
	}ImGui::SameLine();

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

void CommandManager::EditorObjGUI(EditorObj& obj) {
#ifdef _DEBUG
	// 表示名（キャッシュが無ければ初期化）
	if (headerNames.find(obj.id) == headerNames.end()) {
		std::string defaultName = obj.name.empty()
			? "EditorObj" + std::to_string(obj.id)
			: obj.name;

		headerNames[obj.id] = defaultName + "##" + std::to_string(obj.id);
		nameHashes[obj.id] = std::hash<std::string>{}(defaultName);
	}

	if (obj.isActive) {
		bool isOpen = (openedHeaderID == obj.id);
		if (isOpen) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		} else {
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
		}

		// CollapsingHeader 表示
		ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanLabelWidth;
		if (ImGui::CollapsingHeader(headerNames[obj.id].c_str(), base_flags)) {
			ImGui::Indent();
			if (!isOpen) {
				openedHeaderID = obj.id;
			}

			// ImGuiの入力欄で名前編集（最大64文字くらい推奨）
			static char nameBuffer[64]; // 固定長バッファ（使い回し可）
			strncpy_s(nameBuffer, sizeof(nameBuffer), obj.name.c_str(), _TRUNCATE);

			ImGui::Text("Name"); ImGui::SameLine();
			if (ImGui::InputText(obj.inputLabel.c_str(), nameBuffer, sizeof(nameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue)) {
				// Enterで確定された場合のみ name を更新
				obj.name = std::string(nameBuffer);

				// ラベル再生成（空の場合はデフォルト）
				std::string newName = obj.name.empty()
					? "EditorObj" + std::to_string(obj.id)
					: obj.name;

				headerNames[obj.id] = newName + "##" + std::to_string(obj.id);
				nameHashes[obj.id] = std::hash<std::string>{}(newName);
			}

			int objType = static_cast<int>(obj.objType);
			ImGui::Combo("objectType##type", &objType, "Normal\0Player\0Boss\0None\0");
			obj.objType = static_cast<ObjectType>(objType);

			obj.obj->DebugGUI();
			ImGuiManager::ImGuiDragDropButton("Child", "MY_INT",
				[this, &obj](const ImGuiPayload* payload) {
					int receivedValue = *(const int*)payload->Data;
					if (ParentCheck(obj.parent, receivedValue)) {
						obj.childlen.push_back(receivedValue);
						objectList[receivedValue]->parent = obj.id;
						objectList[receivedValue]->obj->SetParent(&obj.obj->transform);
					}
				},
				[this, &obj](const ImGuiPayload* payload) {
					int receivedValue = *(const int*)payload->Data;
					if (ParentCheck(obj.parent, receivedValue)) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
						ImGui::SetTooltip("drop here!");
					} else {
						ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
						ImGui::SetTooltip("cannot drop here");
					}
				});

			if (obj.childlen.size() != 0) {
				for (auto& child : obj.childlen) {
					if (objectList[child]) {
						ImGui::Text(objectList[child]->name.c_str());
					}
				}
			}

			ImGui::Separator();
			JsonSerializer::ShowSaveEditorObjPopup(obj);
			ImGui::SameLine();
			if (ImGui::Button(obj.deleteButtonLabel.c_str())) {
				auto deleteCommand = std::make_unique<DeleteObjCommand>(obj.id);
				Execute(std::move(deleteCommand));
			}
			ImGui::Unindent();
		} else {
			if (isOpen) {
				openedHeaderID.reset();
			}
			ImGui::SameLine();
			ImGuiManager::ImGuiDragButton(obj.dragButtonLabel.c_str(), &obj.id, sizeof(int), "MY_INT");
		}
		if (obj.childlen.size() != 0) {
			ImGui::Indent();
			std::erase_if(obj.childlen, [&](int child) {
				if (objectList[child]) {
					return objectList[child]->parent != obj.id;
				}
				return false;
				});

			for (auto& child : obj.childlen) {
				if (objectList[child]) {
					if (objectList[child]->parent == obj.id) {
						if (!objectList[child]->obj->IsHaveParent()) {
							objectList[child]->obj->SetParent(&obj.obj->transform);
						}
						EditorObjGUI(*objectList[child]);
					}
				}
			}
			ImGui::Unindent();
		}
	} else {

	}
#endif // _DEBUG
}

std::shared_ptr<EditorObj> CommandManager::GetEditorObject(int id) const {
	auto it = objectList.find(id);
	return (it != objectList.end()) ? it->second : nullptr;
}

void CommandManager::GarbageCollect() {
	// 非アクティブなオブジェクトを削除
	std::erase_if(objectList, [](const auto& pair) {
		if (pair.second) {
			return !pair.second->isActive;
		}
		return true;
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
	nlohmann::json jsonList = nlohmann::json::array();

	for (auto& [id, objPtr] : objectList) {
		if (objPtr->parent == -1) {
			jsonList.push_back(ConvertObjToJson(objPtr.get()));
		}
	}

	nlohmann::json finalJson;
	finalJson["objects"] = jsonList;

	std::filesystem::path dir = "resource/Json";
	std::filesystem::create_directories(dir);
	std::filesystem::path fullPath = dir / filePath;

	std::ofstream ofs(fullPath);
	if (ofs.is_open()) {
		ofs << finalJson.dump(4);
		ofs.close();
	}
}

nlohmann::json CommandManager::ConvertObjToJson(EditorObj* obj) {
	nlohmann::json json;

	json["objectName"] = obj->name;
	json["modelName"] = obj->obj->GetModelName();
	switch (obj->objType) {
	case ObjectType::Normal:
		json["objectType"] = "Normal";
		break;
	case ObjectType::Player:
		json["objectType"] = "Player";
		break;
	case ObjectType::Boss:
		json["objectType"] = "Boss";
		break;
	case ObjectType::None:
		json["objectType"] = "None";
		break;
	default:
		break;
	}

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

	// 子供がいる場合は再帰的に追加
	if (!obj->childlen.empty()) {
		nlohmann::json childrenJson = nlohmann::json::array();
		for (int32_t childId : obj->childlen) {
			if (objectList.count(childId) > 0) {
				EditorObj* childObj = objectList[childId].get();
				childrenJson.push_back(ConvertObjToJson(childObj));  // 再帰呼び出し
			}
		}
		json["children"] = childrenJson;
	}

	return json;
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
			StackReset();
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

bool CommandManager::EditorOBJLoad(const std::string& filePath, bool deleteObj) {
	std::filesystem::path fullPath = std::filesystem::path("resource/Json") / filePath;
	if (!std::filesystem::exists(fullPath)) return false;

	std::ifstream ifs(fullPath);
	if (!ifs.is_open()) return false;

	nlohmann::json json;
	try {
		ifs >> json;
	}
	catch (...) {
		return false;
	}
	ifs.close();

	if (!json.contains("objects") || !json["objects"].is_array()) return false;

	if (deleteObj) {
		DeleteEditorObj();
	}

	// 親を持たない root object から再帰的に読み込み
	for (const auto& objJson : json["objects"]) {
		LoadObjRecursive(objJson, -1); // 親なしとして処理
	}

	return true;
}

void CommandManager::LoadObjRecursive(const nlohmann::json& objJson, int parentId) {
	std::string name = objJson.value("objectName", "Unnamed");
	std::string modelName = objJson.value("modelName", "DefaultModel");

	int newId = CommandManager::GetInstance()->nextObjId++;

	auto command = std::make_unique<CreateObjCommand>(newId, name, modelName);
	Execute(std::move(command));

	// 親子関係を構築
	if (parentId != -1) {
		objectList[newId]->parent = parentId;
		objectList[parentId]->childlen.push_back(newId);
	}

	if (objJson.contains("objectType")) {
		if (objJson["objectType"] == "Normal") {
			objectList[newId]->objType = ObjectType::Normal;
		} else if (objJson["objectType"] == "Player") {
			objectList[newId]->objType = ObjectType::Player;
		} else if (objJson["objectType"] == "Boss") {
			objectList[newId]->objType = ObjectType::Boss;
		} else if (objJson["objectType"] == "None") {
			objectList[newId]->objType = ObjectType::None;
		}
	}
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

	// 子オブジェクトの読み込み（再帰）
	if (objJson.contains("children") && objJson["children"].is_array()) {
		for (const auto& childJson : objJson["children"]) {
			LoadObjRecursive(childJson, newId);
		}
	}
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

			if (EditorOBJLoad(path, true)) {
				showLoadSuccessMessage = true;
				StackReset();
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

void CommandManager::DeleteEditorObj() {
	for (auto& group : objectList) {
		EditorObj* obj = group.second.get();
		if (obj) {
			obj->isActive = false;
		}
	}
}

bool CommandManager::ParentCheck(int parentID, int receiveID) {
	if (parentID == receiveID) {
		return false;
	} else if (parentID == -1) {
		return true;
	} else {
		if (objectList[parentID]) {
			return ParentCheck(objectList[parentID]->parent, receiveID);
		}
		return true;
	}
}
