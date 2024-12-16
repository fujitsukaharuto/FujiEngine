#pragma once


class Player;

class ICommand {
public:
	ICommand() = default;
	virtual ~ICommand();


	virtual void Exec(Player& player) = 0;
};


class MoveRightCommand : public ICommand {
public:
	MoveRightCommand() = default;
	~MoveRightCommand()override;


	void Exec(Player& player)override;

private:

};


class MoveLeftCommand : public ICommand {
public:
	MoveLeftCommand() = default;
	~MoveLeftCommand()override;

	void Exec(Player& player)override;

private:

};
