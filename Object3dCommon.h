#pragma once
#include "DXCom.h"
#include "Camera.h"

class Object3dCommon {
public:
	Object3dCommon() = default;
	~Object3dCommon() = default;

public:

	void Initialize(Camera* camera);

	void PreDraw();

	Camera* GetDefaultCamera() const { return defaultCamera_; }

private:



private:

	DXCom* dxCommon_ = nullptr;
	Camera* defaultCamera_ = nullptr;


};
