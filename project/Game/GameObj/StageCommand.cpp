#include "StageCommand.h"
#include "Game/GameObj/Selector/Selector.h"
#include "Game/GameObj/Unit/Unit.h"

SelectorMoveCommand::SelectorMoveCommand(Selector* selector, int x, int y) {
	selector_ = selector;
	x_ = x;
	y_ = y;
}

void SelectorMoveCommand::Exec() {
	selector_->Move(x_, y_);
}

SelectUnitCommand::SelectUnitCommand(Selector* selector) {
	selector_ = selector;
}

void SelectUnitCommand::Exec() {
	selector_->SelectUnit();
}

UnitMoveCommand::UnitMoveCommand(Unit* unit, int x, int y) {
	unit_ = unit;
	x_ = x;
	y_ = y;
}

void UnitMoveCommand::Exec() {
	unit_->Move(x_, y_);
}

UnitMoveEndCommand::UnitMoveEndCommand(Unit* unit, Selector* selector) {
	selector_ = selector;
	unit_ = unit;
}

void UnitMoveEndCommand::Exec() {
	unit_->MoveEnd(selector_);
	selector_->UnSelectUnit();
}
