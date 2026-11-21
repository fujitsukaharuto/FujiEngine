#include "SceneFactory.h"
#include "Game/StarShooter/Scene/TitleScene.h"
#include "Game/StarShooter/Scene/GameScene.h"
#include "Game/StarShooter/Scene/ResultScene.h"
#include "Game/Debug/ParticleDebugScene.h"
#include "Game/Debug/GPUParticleScene.h"
#include "Game/Debug/TestScene.h"
#include "Game/Debug/NightTownScene.h"
#include "Game/Debug/RuinsScene.h"

SceneFactory::SceneFactory() {
	// ここに並べた順がシーン切り替えGUIの表示順になる
	Register<StarShooter::TitleScene>("SS_TITLE");
	Register<StarShooter::GameScene>("SS_GAME");
	Register<StarShooter::ResultScene>("SS_RESULT");
	Register<ParticleDebugScene>("PARTICLEDEBUG");
	Register<GPUParticleScene>("GPUPARTICLE");
	Register<TestScene>("TEST");
	Register<NightTownScene>("NIGHTTOWN");
	Register<RuinsScene>("RUINS");
}

SceneFactory::~SceneFactory() {
}
