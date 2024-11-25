#pragma once
#include "Scene/AbstractSceneFactory.h"


class SceneFactory : public AbstractSceneFactory {
public:
	SceneFactory();
	~SceneFactory();

	BaseScene* CreateScene(const std::string& sceneName)override;

private:

};



