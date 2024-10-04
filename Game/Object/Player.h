#pragma once

//Local
#include"Character.h"

class Player:public Character{
public:

	Player() = default;
	virtual ~Player() override = default;

	void Initialize(std::vector<Model*> models)override;

	void Update()override;

	void Draw()override;

private:

};
