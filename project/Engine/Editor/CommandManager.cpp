#include "CommandManager.h"
#include "Engine/Input/Input.h"

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
				std::string inputLabel = "Name##input" + std::to_string(group.first);

				// ImGuiの入力欄で名前編集（最大64文字くらい推奨）
				static char nameBuffer[64]; // 固定長バッファ（使い回し可）
				strncpy_s(nameBuffer, sizeof(nameBuffer), obj->name.c_str(), _TRUNCATE);

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

				std::string deleteButtonLabel = "Delete##" + std::to_string(group.first);
				if (ImGui::Button(deleteButtonLabel.c_str())) {
					auto deleteCommand = std::make_unique<DeleteObjCommand>(group.first);
					Execute(std::move(deleteCommand));
					continue;
				}

				obj->obj->DebugGUI();
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
