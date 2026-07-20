#pragma once
#include "Engine/Editor/ICommand.h"

#include <string>
#include <memory>


namespace Editor {

	struct EditorObj;

	/// <summary>
	/// EditorObjectを生み出すコマンドクラス
	/// </summary>
	class CreateObjCommand : public ICommand {
	public:
		/// <summary>生成するオブジェクトの情報を受け取る</summary>
		/// <param name="id">オブジェクトのID</param> <param name="name">オブジェクトの名前</param> <param name="mName">モデルの名前</param>
		CreateObjCommand(int id, const std::string& name, const std::string& mName = "Sphere")
			: objId(id), objName(name), modelName(mName) {
		}

		/// <summary>Object3dを生成してオブジェクトリストへ登録する</summary>
		void Do() override;

		/// <summary>非アクティブにして描画されないようにする</summary>
		void UnDo() override;

		/// <summary>アクティブに戻してオブジェクトリストへ再登録する</summary>
		void ReDo() override;


	private:

		int objId;
		std::string objName;
		std::string modelName;
		std::shared_ptr<EditorObj> obj;

	};

}
