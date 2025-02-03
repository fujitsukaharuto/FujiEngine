#pragma once
#include "Game/OriginGameObject.h"

class Selector;

class Unit : public OriginGameObject {
public:
	Unit() = default;
	~Unit()override;

public:

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void Move(int x, int y);
	void MoveEnd(Selector* selector);

	int GetPosX() { return int(model_->transform.translate.x); }
	int GetPosY() { return int(model_->transform.translate.y); }

private:

	int mapX_ = 0;
	int mapY_ = 0;
	int mapSize_ = 32;
	int id_ = 0;
	bool isSelected_ = false;

};
