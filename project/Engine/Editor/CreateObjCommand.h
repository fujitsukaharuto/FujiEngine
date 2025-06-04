#pragma once
#include "Engine/Editor/ICommand.h"

#include <memory>


struct EditorObj;

class CreateObjCommand : public ICommand {
public:
	CreateObjCommand(int id) : objId(id) {
	}

	void Do() override;

	void UnDo() override;

	void ReDo() override;

private:

	int objId;
	std::shared_ptr<EditorObj> obj;

};