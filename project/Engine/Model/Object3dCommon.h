#pragma once
#include "Engine/Math/Vector/Vector2.h"

class DXCom;
class LightManager;

struct PickingBuffer {
	int objID;
	float depth;
};

struct PickingData {
	Vector2 pickingPixelCoord;
	uint32_t pickingEnable;
};


class Object3dCommon {
public:
	Object3dCommon() = default;
	~Object3dCommon() = default;

public:

	void Initialize();

	void PreDraw();

	void PreAnimationDraw();

private:



private:

	DXCom* dxcommon_ = nullptr;
	LightManager* lightManager_;

};
