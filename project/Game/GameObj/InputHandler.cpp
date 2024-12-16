#include "InputHandler.h"
#include "Input/Input.h"

InputHandler::~InputHandler() {
}

ICommand* InputHandler::HandleInput() {
	if (Input::GetInstance()->TriggerKey(DIK_D)) {
		ctrlZList_.push_back("A");
		return pressKeyD_;
	}
	if (Input::GetInstance()->TriggerKey(DIK_A)) {
		ctrlZList_.push_back("D");
		return pressKeyA_;
	}
	if (Input::GetInstance()->TriggerKey(DIK_Z) && Input::GetInstance()->PushKey(DIK_LCONTROL)) {
		if (ctrlZList_.empty()) {
			return nullptr;
		}
		std::string execKey = ctrlZList_.back();
		ctrlZList_.pop_back();
		if (execKey == "D") {
			return pressKeyD_;
		}
		if (execKey == "A") {
			return pressKeyA_;
		}
	}

	return nullptr;
}

void InputHandler::AssignMoveLeftCommand2PressKeyA() {
	ICommand* command = new MoveLeftCommand();
	this->pressKeyA_ = command;
}

void InputHandler::AssignMoveRightCommand2PressKeyD() {
	ICommand* command = new MoveRightCommand();
	this->pressKeyD_ = command;
}
