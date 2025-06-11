#include "DeleteObjCommand.h"
#include "Engine/Editor/CommandManager.h"

DeleteObjCommand::DeleteObjCommand(int id) : objId(id) {
	obj = CommandManager::GetInstance()->GetEditorObject(id);
}

void DeleteObjCommand::Do() {
	// オブジェクトを描画しないように
	if (obj) {
		obj->isActive = false;
	}
}

void DeleteObjCommand::UnDo() {
	// 描画するように
	if (obj) {
		obj->isActive = true;
		CommandManager::GetInstance()->objectList[objId] = obj;
	}
}

void DeleteObjCommand::ReDo() {
	if (obj) {
		obj->isActive = false;
	}
}
