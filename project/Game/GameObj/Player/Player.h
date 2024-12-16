#pragma once
#include "OriginGameObject.h"

class Player : public OriginGameObject {
public:
	Player();
	~Player()override;

public:

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void MoveRight();
	void MoveLeft();

private:



private:

	float moveSpeed_ = 1.0f;


};
