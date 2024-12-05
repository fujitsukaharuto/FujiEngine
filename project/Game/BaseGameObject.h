#pragma once
#include <memory>
#include "Model/Object3d.h"


class BaseGameObject {
public:
	BaseGameObject();
	virtual ~BaseGameObject() = default;

	virtual void Initialize();
	virtual void Update();
	virtual void Draw();

	virtual void Debug();

public:



private:

	std::unique_ptr<Object3d> model_;



};
