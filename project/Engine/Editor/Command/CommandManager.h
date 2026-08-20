#pragma once
#include <stack>
#include <unordered_map>
#include <optional>
#include <memory>
#include <string>
#include <cstring>
#include <json_fwd.hpp>	// json は宣言にしか使わないので前方宣言で足りる

#include "Engine/Editor/Command/ICommand.h"
#include "Engine/Editor/Command/PropertyCommand.h"
#include "Engine/Editor/Command/CreateObjCommand.h"
#include "Engine/Editor/Command/DeleteObjCommand.h"

#include "Engine/Graphics/Object/Object3d.h"

namespace Editor {

	/// <summary>
	/// EditorObjectの種類
	/// </summary>
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

		/// <summary>
		/// インスタンスの取得
		/// </summary>
		static CommandManager* GetInstance() {
			static CommandManager instance;
			return &instance;
		}

	public:
		/// <summary>
		/// コマンドを実行してUndoスタックへ積む
		/// </summary>
		void Execute(std::unique_ptr<ICommand> command);

		/// <summary>
		/// 直前のコマンドを取り消してRedoスタックへ移す
		/// </summary>
		void Undo();

		/// <summary>
		/// 取り消したコマンドをやり直してUndoスタックへ戻す
		/// </summary>
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
		/// <summary>
		/// スタックとオブジェクトリストを空にする
		/// </summary>
		void Reset();

		/// <summary>
		/// Undo / Redoのスタックだけを空にする
		/// </summary>
		void StackReset();

		/// <summary>
		/// 終了処理、保持しているデータをすべて解放する
		/// </summary>
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


		/// <summary>
		/// アクティブなEditorObjectをすべて描画
		/// </summary>
		void Draw();

		/// <summary>
		/// ルートのEditorObjectを並べたデバッグGUI
		/// </summary>
		void DebugGUI();

		/// <summary>
		/// EditorObject1つ分のGUI、子オブジェクトも再帰的に表示する
		/// </summary>
		void EditorObjGUI(EditorObj& obj);

		/// <summary>
		/// IDからEditorObjectを取得
		/// </summary>
		std::shared_ptr<EditorObj> GetEditorObject(int id) const;

		/// <summary>
		/// 非アクティブなEditorObjectと、それに紐づく名前やハッシュを破棄する
		/// </summary>
		void GarbageCollect();

		/// <summary>
		/// EditorObjectのリストを取得
		/// </summary>
		std::unordered_map<int, std::shared_ptr<EditorObj>>& GetObjectList() { return objectList; }

		/// <summary>
		/// ヘッダーの表示名のリストを取得
		/// </summary>
		std::unordered_map<int, std::string>& GetHeaderNames() { return headerNames; }

		/// <summary>
		/// 表示名のハッシュのリストを取得
		/// </summary>
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
