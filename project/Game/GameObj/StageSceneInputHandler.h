#pragma once
#include "Game/GameObj/StageCommand.h"

class StageSceneInputHandler {
public:
	StageSceneInputHandler();
	~StageSceneInputHandler();

public:

	IStageCommand* selectorHandleInput(Selector* selector);
	IStageCommand* UnitHandleInput(Unit* unit);

	bool CheckUndoStatus();

	void SetSelector(Selector* selector) { selector_ = selector; }

private:

	Selector* selector_;
	Unit* selectedUnit_;

};
