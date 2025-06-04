#include "CreateObjCommand.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Model/Object3d.h"

void CreateObjCommand::Do() {
	// オブジェクトを作ってListへ
	obj = std::make_shared<EditorObj>();
	obj->id = objId;
	obj->isActive = true;
	obj->obj = std::make_unique<Object3d>();
	obj->obj->CreateSphere();
	CommandManager::GetInstance()->objectList[objId] = obj;
}

void CreateObjCommand::UnDo() {
	// 描画しないように
	obj->isActive = false;
}

void CreateObjCommand::ReDo() {
	obj->isActive = true;
}
