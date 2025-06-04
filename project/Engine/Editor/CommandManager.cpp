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
}

void CommandManager::Finalize() {
	undoStack = std::stack<std::unique_ptr<ICommand>>();
	redoStack = std::stack<std::unique_ptr<ICommand>>();

	objectList.clear();
}

std::shared_ptr<EditorObj> CommandManager::GetEditorObject(int id) const {
	auto it = objectList.find(id);
	return (it != objectList.end()) ? it->second : nullptr;
}

void CommandManager::GarbageCollect() {
}
