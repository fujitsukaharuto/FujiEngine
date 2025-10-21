#pragma once
#include "BaseScene.h"
#include <string>

/// <summary>
/// シーンファクトリー基底クラス
/// </summary>
class AbstractSceneFactory {
public:
	virtual ~AbstractSceneFactory() = default;
	virtual BaseScene* CreateScene(const std::string& sceneName) = 0;

private:

};
