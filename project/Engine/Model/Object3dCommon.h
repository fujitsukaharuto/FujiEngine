#pragma once

class DXCom;

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

};
