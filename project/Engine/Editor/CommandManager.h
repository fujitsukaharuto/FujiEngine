#pragma once
#include <stack>
#include <memory>
#include <string>
#include <cstring>
#include <json.hpp>

#include "Engine/Editor/ICommand.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/CreateObjCommand.h"
#include "Engine/Editor/DeleteObjCommand.h"

#include "Engine/Model/Object3d.h"

enum class ObjectType{
	Normal,
	Player,
	Boss,
	None,
};

struct EditorObj {
	int id;
	std::unique_ptr<Object3d> obj;
	ObjectType objType;
	bool isActive = true;
	std::string name;
	std::string modelName;
	std::vector<int> childlen;
	int parent = -1;

	std::string inputLabel;
	std::string deleteButtonLabel;
	std::string dragButtonLabel;
};

struct LoadEditorObjData {
	std::string name;
	std::string modelName;
	Trans trnasform;
	ObjectType objType;
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
	void StackReset();
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
	void EditorObjGUI(EditorObj& obj);
	std::shared_ptr<EditorObj> GetEditorObject(int id) const;
	void GarbageCollect();

	std::unordered_map<int, std::shared_ptr<EditorObj>> objectList;
	std::unordered_map<int, std::string> headerNames;
	std::unordered_map<int, size_t> nameHashes;

private:

	void EditorOBJSave(const std::string& filePath);
	nlohmann::json ConvertObjToJson(EditorObj* obj);
	void SaveAllEditorOBJ();
	bool EditorOBJLoad(const std::string& filePath, bool deleteObj = false);
	void LoadObjRecursive(const nlohmann::json& objJson, int parentId);
	void LoadAllEditorOBJ();

	void DeleteEditorObj();
	bool ParentCheck(int parentID, int receiveID);

private:
	std::stack<std::unique_ptr<ICommand>> undoStack;
	std::stack<std::unique_ptr<ICommand>> redoStack;

	int nextObjId = 0;
	std::unique_ptr<EditorObj> loadObj;

	std::optional<int> openedHeaderID;
};
