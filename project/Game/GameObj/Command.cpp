#include "Command.h"
#include "Game/GameObj/Player/Player.h"


ICommand::~ICommand() {
}


MoveRightCommand::~MoveRightCommand() {
}

void MoveRightCommand::Exec(Player& player) {
	player.MoveRight();
}



MoveLeftCommand::~MoveLeftCommand() {
}

void MoveLeftCommand::Exec(Player& player) {
	player.MoveLeft();
}
