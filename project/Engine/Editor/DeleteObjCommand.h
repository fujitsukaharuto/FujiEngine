#pragma once
#include "Engine/Editor/ICommand.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector3.h"

class DeleteObjCommand : public ICommand {
public:


	DeleteObjCommand(int id) : objId(id) {
	}

	void Do() override {
		// オブジェクトを描画しないように
	}

	void UnDo() override {
		// 描画するように
	}

	void ReDo() override {
		Do();
	}

private:

	int objId;

};