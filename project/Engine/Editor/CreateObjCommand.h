#pragma once
#include "Engine/Editor/ICommand.h"

#include <string>
#include <memory>


struct EditorObj;

class CreateObjCommand : public ICommand {
public:
	CreateObjCommand(int id, const std::string& name)
		: objId(id), objName(name) {
	}

	void Do() override;

	void UnDo() override;

	void ReDo() override;


private:

	int objId;
	std::string objName;
	std::shared_ptr<EditorObj> obj;

};