#include "SceneFactory.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Scene/GameScene.h"
#include "Game/Scene/ResultScene.h"
#include "Game/Scene/ParticleDebugScene.h"
#include "Game/Scene/GPUParticleScene.h"
#include "Game/Scene/TestScene.h"

SceneFactory::SceneFactory() {
	// ここに並べた順がシーン切り替えGUIの表示順になる
	Register<TitleScene>("TITLE");
	Register<GameScene>("GAME");
	Register<ResultScene>("RESULT");
	Register<ParticleDebugScene>("PARTICLEDEBUG");
	Register<GPUParticleScene>("GPUPARTICLE");
	Register<TestScene>("TEST");
}

SceneFactory::~SceneFactory() {
}
