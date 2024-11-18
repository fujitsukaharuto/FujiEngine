#pragma once
#include "DXCom.h"

class Object3dCommon {
public:
	Object3dCommon() = default;
	~Object3dCommon() = default;

public:

	void Initialize();

	void PreDraw();


private:



private:

	DXCom* dxCommon_ = nullptr;

};
