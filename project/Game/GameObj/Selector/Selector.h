#pragma once
#include "Game/GameObj/Unit/Unit.h"

class Selector {
public:
	Selector();
	~Selector();

public:

	void Init();
	void Update();
	void Draw();

	void Move(int x, int y);
	void SelectUnit();

private:

	int mapX_;
	int mapY_;
	int mapSize_;

	int selectMode_;

	Unit* unit_;
	Unit* selectedUnit_;

};
