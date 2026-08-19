#include "GameRun.h"
#include "Game/Scene/SceneFactory.h"
#include "Game/Particle/GameEmitters.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Editor;
using namespace Scene;


GameRun::GameRun() {
}

GameRun::~GameRun() {
}

void GameRun::Initialize() {
	// リソースをここであらかじめLoadしておく
#pragma region リソース読み込み

#pragma region テクスチャ読み込み
	textureManager_->LoadAll();
#pragma endregion

#pragma region オブジェクト読み込み
	ModelManager::GetInstance()->CreateSphere();
	// resource/Models 内の.obj/.gltfを全て読み込む(T_boss.gltfもここに含まれる)
	modelManager_->LoadAllFileData();
#pragma endregion

#pragma region パーティクル生成
	
	LoadParticleGroup();
	Game::CreateDefaultEmitters();

#pragma endregion

#pragma region サウンド読み込み

	LoadSoundData();

#pragma endregion

	GlobalVariables::GetInstance()->LoadFiles();

#pragma endregion

	sceneFactory_ = std::make_unique<SceneFactory>();
	sceneManager_->SetFactory(sceneFactory_.get());

	int sceneNum = 0;
	if (sceneNum == 0) {
		sceneManager_->StartScene("TEST");
	} else {
		sceneManager_->StartScene("TITLE");
	}

}

void GameRun::LoadParticleGroup() {
	// パーティクルグループの生成情報は resource/ParticleGroups/*.json が持っている。
	// テクスチャ・数・形状・ペアレント版の要否まで全てJSON側で完結する
	pManager_->LoadAllFileData();

	// 連番アニメだけはJSON走査の対象外なのでここで生成する
	pManager_->CreateAnimeGroup("animetest", "uvChecker.png");
	pManager_->AddAnime("animetest", "white2x2.png", 10.0f);
}

void GameRun::LoadSoundData() {
	audioPlayer_->LoadAll();
}
