#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <vector>
#include "Engine/Math/Vector/Vector2.h"

class DXCom;
class LightManager;

/// <summary>
/// Object関連の汎用処理をするクラス
/// </summary>
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
