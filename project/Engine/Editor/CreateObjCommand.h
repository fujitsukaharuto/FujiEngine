#pragma once
#include "Engine/Editor/ICommand.h"

#include <string>
#include <memory>


struct EditorObj;

class CreateObjCommand : public ICommand {
public:
	CreateObjCommand(int id, const std::string& name, const std::string& mName = "Sphere")
		: objId(id), objName(name), modelName(mName) {
	}

	void Do() override;

	void UnDo() override;

	void ReDo() override;


private:

	int objId;
	std::string objName;
	std::string modelName;
	std::shared_ptr<EditorObj> obj;

};