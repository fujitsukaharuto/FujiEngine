#pragma once
#include "Scene/AbstractSceneFactory.h"

enum class SceneNames {
	TITLE,
	GAME,
	RESULT,
	PARTICLEDEBUG,
	GPUPARTICLE,
	TEST
};

/// <summary>
/// シーンファクトリー
/// </summary>
class SceneFactory : public Scene::AbstractSceneFactory {
public:
	SceneFactory();
	~SceneFactory();

	/// <summary>
	/// シーンの作成を行う
	/// </summary>
	std::unique_ptr<Scene::BaseScene> CreateScene(const std::string& sceneName) override;
	std::vector<std::string> GetSceneNames() const override;

private:

};



