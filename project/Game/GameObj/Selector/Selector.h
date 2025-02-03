#pragma once
#include "Game/OriginGameObject.h"
#include "Game/GameObj/Unit/Unit.h"

enum SelectMode {
	SELECTOR,
	UNIT,
};

class Selector : public OriginGameObject {
public:
	Selector();
	~Selector()override;

public:

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void Move(int x, int y);
	void SelectUnit();
	void UnSelectUnit();

	int GetSelectMode() { return selectMode_; }
	Unit* GetSelectedUnitAddress() { return selectedUnit_; }

	int GetPosX() { return int(model_->transform.translate.x); }
	int GetPosY() { return int(model_->transform.translate.y); }

private:

	int mapX_;
	int mapY_;
	int mapSize_;

	int selectMode_;

	Unit* unit_;
	Unit* selectedUnit_;

};
