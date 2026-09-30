#pragma once
#include <stack>
#include <memory>
#include <string>

#include "Engine/Editor/Command/ICommand.h"
#include "Engine/Editor/Command/PropertyCommand.h"

namespace Editor {

	/// <summary>
	/// 編集操作の Undo / Redo を管理するクラス
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
		/// Undo / Redo のスタックを空にする
		/// </summary>
		/// <remarks>コマンドは Transform を参照で握るので、対象が消える前(シーン切り替え・置物の削除)に呼ぶこと</remarks>
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


	private:
		std::stack<std::unique_ptr<ICommand>> undoStack;
		std::stack<std::unique_ptr<ICommand>> redoStack;
	};

}
