#pragma once
#include <stack>
#include <memory>
#include <string>
#include <cstring>

#include "Engine/Editor/ICommand.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/CreateObjCommand.h"
#include "Engine/Editor/DeleteObjCommand.h"

#include "Engine/Model/Object3d.h"


struct EditorObj {
	int id;
	std::unique_ptr<Object3d> obj;
	bool isActive = true;
	std::string name;
};

class CommandManager {
public:
	CommandManager() = default;
	~CommandManager();
	CommandManager(const CommandManager&) = delete;
	CommandManager& operator=(const CommandManager&) = delete;

	static CommandManager* GetInstance() {
		static CommandManager instance;
		return &instance;
	}

public:
	void Execute(std::unique_ptr<ICommand> command);
	void Undo();
	void Redo();

	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }

	void CheckInputForUndoRedo();
	void Reset();
	void Finalize();


	template<typename T>
	static void TryCreatePropertyCommand(Trans& trans, const T& prevValue, T& currentValue, T Trans::* member) {
		if (currentValue != prevValue) {
			auto command = std::make_unique<PropertyCommand<T>>(trans, member, prevValue, currentValue);
			GetInstance()->Execute(std::move(command));
		}
	}


	void Draw();
	void DebugGUI();
	std::shared_ptr<EditorObj> GetEditorObject(int id) const;
	void GarbageCollect();

	std::unordered_map<int, std::shared_ptr<EditorObj>> objectList;
	std::unordered_map<int, std::string> headerNames;
	std::unordered_map<int, size_t> nameHashes;

private:
	std::stack<std::unique_ptr<ICommand>> undoStack;
	std::stack<std::unique_ptr<ICommand>> redoStack;

	int nextObjId = 0;

};
