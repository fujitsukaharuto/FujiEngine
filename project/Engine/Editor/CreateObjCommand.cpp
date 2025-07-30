#include "CreateObjCommand.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Model/Object3d.h"

void CreateObjCommand::Do() {
	// オブジェクトを作ってListへ
	obj = std::make_shared<EditorObj>();
	obj->id = objId;
	obj->isActive = true;
	obj->name = objName;
	obj->obj = std::make_unique<Object3d>();
	obj->obj->Create(modelName);
	obj->obj->SetEditorObjParameter();
	obj->modelName = obj->obj->GetModelName();

	obj->inputLabel = "##input" + std::to_string(obj->id);
	obj->deleteButtonLabel = "Delete##" + std::to_string(obj->id);
	obj->dragButtonLabel = "p##" + std::to_string(obj->id);

	CommandManager::GetInstance()->objectList[objId] = obj;

	// ラベルとハッシュの登録もここでやっておく
	std::string labelName = objName.empty()
		? "EditorObj" + std::to_string(objId)
		: objName;
	CommandManager::GetInstance()->headerNames[objId] = labelName + "##" + std::to_string(objId);
	CommandManager::GetInstance()->nameHashes[objId] = std::hash<std::string>{}(labelName);

}

void CreateObjCommand::UnDo() {
	// 描画しないように
	obj->isActive = false;
}

void CreateObjCommand::ReDo() {
	obj->isActive = true;
	// 再登録
	CommandManager::GetInstance()->objectList[objId] = obj;
}
