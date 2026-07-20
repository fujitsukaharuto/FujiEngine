#pragma once
#include "Engine/Editor/ICommand.h"

#include <memory>


namespace Editor {

	struct EditorObj;

	/// <summary>
	/// EditorObjectを削除するコマンドクラス
	/// </summary>
	class DeleteObjCommand : public ICommand {
	public:


		/// <summary>削除するオブジェクトのIDを受け取る</summary>
		/// <param name="id">オブジェクトのID</param>
		DeleteObjCommand(int id);

		/// <summary>非アクティブにして描画されないようにする</summary>
		void Do() override;

		/// <summary>アクティブに戻してオブジェクトリストへ再登録する</summary>
		void UnDo() override;

		/// <summary>もう一度非アクティブにする</summary>
		void ReDo() override;

	private:

		int objId;
		std::shared_ptr<EditorObj> obj;

	};

}
