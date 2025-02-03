#include "StageSceneInputHandler.h"
#include "Engine/Input/Input.h"
#include "Game/GameObj/Selector/Selector.h"

StageSceneInputHandler::StageSceneInputHandler() {
}

StageSceneInputHandler::~StageSceneInputHandler() {
}

IStageCommand* StageSceneInputHandler::selectorHandleInput(Selector* selector) {
	
	IStageCommand* command = nullptr;

	if (Input::GetInstance()->TriggerKey(DIK_D)) {
		command = new SelectorMoveCommand(selector, selector->GetPosX() + 1, selector->GetPosY());
	} else if (Input::GetInstance()->TriggerKey(DIK_A)) {
		command = new SelectorMoveCommand(selector, selector->GetPosX() - 1, selector->GetPosY());
	} else if (Input::GetInstance()->TriggerKey(DIK_W)) {
		command = new SelectorMoveCommand(selector, selector->GetPosX(), selector->GetPosY() + 1);
	} else if (Input::GetInstance()->TriggerKey(DIK_S)) {
		command = new SelectorMoveCommand(selector, selector->GetPosX(), selector->GetPosY() - 1);
	}
	
	return command;
}

IStageCommand* StageSceneInputHandler::UnitHandleInput(Unit* unit) {
	
	IStageCommand* command = nullptr;

	if (Input::GetInstance()->TriggerKey(DIK_D)) {
		command = new UnitMoveCommand(unit, unit->GetPosX() + 1, unit->GetPosY());
	}
	else if (Input::GetInstance()->TriggerKey(DIK_A)) {
		command = new UnitMoveCommand(unit, unit->GetPosX() - 1, unit->GetPosY());
	}
	else if (Input::GetInstance()->TriggerKey(DIK_W)) {
		command = new UnitMoveCommand(unit, unit->GetPosX(), unit->GetPosY() + 1);
	}
	else if (Input::GetInstance()->TriggerKey(DIK_S)) {
		command = new UnitMoveCommand(unit, unit->GetPosX(), unit->GetPosY() - 1);
	}
	
	return command;
}

bool StageSceneInputHandler::CheckUndoStatus() {
	return false;
}
