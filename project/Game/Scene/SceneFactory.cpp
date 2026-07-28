#include "SceneFactory.h"
#include "Game/Scene/Main/TitleScene.h"
#include "Game/Scene/Main/GameScene.h"
#include "Game/Scene/Main/ResultScene.h"
#include "Game/Scene/Debug/ParticleDebugScene.h"
#include "Game/Scene/Debug/GPUParticleScene.h"
#include "Game/Scene/Debug/TestScene.h"

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
