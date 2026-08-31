#pragma once
#include "Engine/Scene/AbstractSceneFactory.h"

/// <summary>
/// シーンファクトリー。追加はコンストラクタに Register 行を1つ足すだけでよい
/// </summary>
class SceneFactory : public Scene::AbstractSceneFactory {
public:
	SceneFactory();
	~SceneFactory();

private:

};
