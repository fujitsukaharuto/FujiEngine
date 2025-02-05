#pragma once

#include"Sprite.h"

class BackGround {
private:
	std::unique_ptr<Sprite>sprite_

public:
	void Init();
	void Update();
	void Draw();
};