#pragma once
#include "Engine/Editor/ICommand.h"

#include <memory>


struct EditorObj;

class DeleteObjCommand : public ICommand {
public:


	DeleteObjCommand(int id);

	void Do() override;

	void UnDo() override;

	void ReDo() override;

private:

	int objId;
	std::shared_ptr<EditorObj> obj;

};