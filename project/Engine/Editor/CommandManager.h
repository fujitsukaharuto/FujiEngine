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

namespace Editor {

	enum class ObjectType{
		Normal,
		Player,
		Boss,
		None,
	};

	/// <summary>
	/// EditorObject用のデータ構造体
	/// </summary>
	struct EditorObj {
		int id;
		std::unique_ptr<Graphics::Object3d> obj;
		ObjectType objType;
		bool isActive = true;
		std::string name;
		std::string modelName;
		std::vector<int> children;
		int parent = -1;

		std::string inputLabel;
		std::string deleteButtonLabel;
		std::string dragButtonLabel;
	};

	/// <summary>
	/// EditorObjectのLoad時用のデータ
	/// </summary>
	struct LoadEditorObjData {
		std::string name;
		std::string modelName;
		Math::Trans transform;
		ObjectType objType;
	};


	/// <summary>
	/// EditorObjectのUndo、Redoをするためのクラス
	/// </summary>
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

		/// <summary>
		/// Undoができるのかのチェック
		/// </summary>
		bool CanUndo() const { return !undoStack.empty(); }

		/// <summary>
		/// Redoできるのかのチェック
		/// </summary>
		bool CanRedo() const { return !redoStack.empty(); }


		/// <summary>
		/// 入力状態を確認し、Undo / Redo 操作を実行する
		/// </summary>
		void CheckInputForUndoRedo();

		//========================================================================*/
		//* データリセット用関数群
		void Reset();
		void StackReset();
		void Finalize();

		/// <summary>
		/// Commandの作成テンプレート
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="trans">位置</param>
		/// <param name="prevValue">前の値</param>
		/// <param name="currentValue">今の値</param>
		/// <param name="member">メンバーの位置</param>
		template<typename T>
		static void TryCreatePropertyCommand(Math::Trans& trans, const T& prevValue, T& currentValue, T Math::Trans::* member) {
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

		std::unordered_map<int, std::shared_ptr<EditorObj>>& GetObjectList() { return objectList; }
		std::unordered_map<int, std::string>& GetHeaderNames() { return headerNames; }
		std::unordered_map<int, size_t>& GetNameHashes() { return nameHashes; }

	private:

		/// <summary>
		/// データのセーブ
		/// </summary>
		void EditorOBJSave(const std::string& filePath);

		/// <summary>
		/// ObjectのデータをJsonに書き込む
		/// </summary>
		nlohmann::json ConvertObjToJson(EditorObj* obj);

		/// <summary>
		/// EditorObjectのセーブ
		/// </summary>
		void SaveAllEditorOBJ();

		/// <summary>
		/// EditorObjectのロード
		/// </summary>
		bool EditorOBJLoad(const std::string& filePath, bool deleteObj = false);

		/// <summary>
		/// JSONデータからエディタ用オブジェクトを再帰的に読み込む
		/// </summary>
		void LoadObjRecursive(const nlohmann::json& objJson, int parentId);

		/// <summary>
		/// すべてのエディタ用オブジェクトをロード
		/// </summary>
		void LoadAllEditorOBJ();

		/// <summary>
		/// エディタ上に存在するすべてのオブジェクトを削除
		/// </summary>
		void DeleteEditorObj();

		/// <summary>
		/// 指定したオブジェクトが、他のオブジェクトの親子関係上で不正な関連を持たないか確認
		/// </summary>
		bool ParentCheck(int parentID, int receiveID);

	private:
		std::unordered_map<int, std::shared_ptr<EditorObj>> objectList;
		std::unordered_map<int, std::string> headerNames;
		std::unordered_map<int, size_t> nameHashes;

		std::stack<std::unique_ptr<ICommand>> undoStack;
		std::stack<std::unique_ptr<ICommand>> redoStack;

		int nextObjId = 0;
		std::unique_ptr<EditorObj> loadObj;

		std::optional<int> openedHeaderID;
	};

}
