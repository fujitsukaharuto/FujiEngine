#pragma once
#include "Engine/Editor/ICommand.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector3.h"

class MoveCommand : public ICommand {
public:
	MoveCommand() = default;
	~MoveCommand()override = default;

public:
	MoveCommand(Trans& target, const Vector3& oldPosition, const Vector3& newPosition)
		: target_(target), oldPosition_(oldPosition), newPosition_(newPosition) {
	}

	void Do() override {
		target_.translate = newPosition_;
	}

	void UnDo() override {
		target_.translate = oldPosition_;
	}

	void ReDo() override {
		Do();
	}

private:
	Trans& target_;
	Vector3 oldPosition_;
	Vector3 newPosition_;
};
