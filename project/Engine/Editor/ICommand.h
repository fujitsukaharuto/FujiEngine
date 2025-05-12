#pragma once


class ICommand {
public:

	virtual ~ICommand() = default;
	virtual void Do() = 0;
	virtual void UnDo() = 0;
	virtual void ReDo() = 0;

};
