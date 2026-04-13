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
class SceneFactory : public AbstractSceneFactory {
public:
	SceneFactory();
	~SceneFactory();

	/// <summary>
	/// シーンの作成を行う
	/// </summary>
	std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;

private:

};



