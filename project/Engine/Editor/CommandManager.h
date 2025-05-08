#pragma once
#include <stack>
#include <memory>

#include "Engine/Editor/ICommand.h"


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
	void Finalize();

private:
	std::stack<std::unique_ptr<ICommand>> undoStack;
	std::stack<std::unique_ptr<ICommand>> redoStack;
};
