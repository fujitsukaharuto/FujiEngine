#pragma once




class OriginState {
public:
	OriginState() = default;
	virtual ~OriginState() = default;

	virtual void Initialize() = 0;
	virtual void Update() = 0;

private:

};
