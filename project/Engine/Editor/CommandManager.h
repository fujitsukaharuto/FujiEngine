#pragma once
#include <stack>
#include <memory>

#include "Engine/Editor/ICommand.h"
#include "Engine/Editor/PropertyCommand.h"

#include "Engine/Model/Object3d.h"


struct EditorObj {
	int id;
	std::unique_ptr<Object3d> obj;
	bool isActive = true;
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


	std::shared_ptr<EditorObj> GetEditorObject(int id) const;
	void GarbageCollect();

	std::unordered_map<int, std::shared_ptr<EditorObj>> objectList;

private:
	std::stack<std::unique_ptr<ICommand>> undoStack;
	std::stack<std::unique_ptr<ICommand>> redoStack;

	int nextObjId = 0;

};
