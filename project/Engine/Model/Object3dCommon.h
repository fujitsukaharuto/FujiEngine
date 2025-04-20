#pragma once

class DXCom;
class LightManager;

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
