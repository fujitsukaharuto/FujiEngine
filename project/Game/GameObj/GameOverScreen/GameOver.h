#pragma once

#include"Sprite.h"

class GameOver {
private:
	std::unique_ptr<Sprite> overPaneru_;
public:
	void Init();
	void Update();
	void Draw();
};