#pragma once


class Selector;
class Unit;

class IStageCommand {
public:
	IStageCommand() = default;
	virtual ~IStageCommand() = default;
	virtual void Exec() = 0;
};



class SelectorMoveCommand : public IStageCommand {
public:
	SelectorMoveCommand(Selector* selector, int x, int y);
	~SelectorMoveCommand()override = default;

public:

	void Exec() override;

private:

	Selector* selector_;
	int x_;
	int y_;
};



class SelectUnitCommand : public IStageCommand {
public:
	SelectUnitCommand(Selector* selector);
	~SelectUnitCommand()override = default;

public:

	void Exec()override;

private:

	Selector* selector_;

};



class UnitMoveCommand : public IStageCommand {
public:
	UnitMoveCommand(Unit* unit, int x, int y);
	~UnitMoveCommand()override = default;

public:

	void Exec()override;

private:

	Unit* unit_;
	int x_;
	int y_;

};


class UnitMoveEndCommand : public IStageCommand {
public:
	UnitMoveEndCommand(Unit* unit, Selector* selector);
	~UnitMoveEndCommand()override = default;

public:

	void Exec()override;

private:

	Unit* unit_;
	Selector* selector_;

};

