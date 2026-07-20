#include "SceneFactory.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Scene/GameScene.h"
#include "Game/Scene/ResultScene.h"
#include "Game/Scene/ParticleDebugScene.h"
#include "Game/Scene/GPUParticleScene.h"
#include "Game/Scene/TestScene.h"

using namespace Scene;

SceneFactory::SceneFactory() {
}

SceneFactory::~SceneFactory() {
}

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) {
	std::unique_ptr<BaseScene> newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = std::make_unique<TitleScene>();

	} else if (sceneName == "GAME") {
		newScene = std::make_unique<GameScene>();

	} else if (sceneName == "RESULT") {
		newScene = std::make_unique<ResultScene>();

	} else if (sceneName == "PARTICLEDEBUG") {
		newScene = std::make_unique<ParticleDebugScene>();

	} else if (sceneName == "GPUPARTICLE") {
		newScene = std::make_unique<GPUParticleScene>();

	} else if (sceneName == "TEST") {
		newScene = std::make_unique<TestScene>();

	}

	return newScene;
}

std::vector<std::string> SceneFactory::GetSceneNames() const {
	return { "TITLE","GAME","RESULT","PARTICLEDEBUG" ,"GPUPARTICLE" ,"TEST" };
}
