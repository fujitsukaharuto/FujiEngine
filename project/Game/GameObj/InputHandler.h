#pragma once
#include <list>
#include <string>
#include "Game/GameObj/Command.h"


class InputHandler {
public:
	InputHandler() = default;
	~InputHandler();

public:

	ICommand* HandleInput();

	void AssignMoveLeftCommand2PressKeyA();
	void AssignMoveRightCommand2PressKeyD();

private:


private:

	ICommand* pressKeyD_;
	ICommand* pressKeyA_;

	std::list <std::string> ctrlZList_;

};
