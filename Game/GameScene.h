#pragma once
#include "Audio.h"
#include "DXCom.h"
#include "Input.h"
#include "DebugCamera.h"
#include "Model.h"


class GameScene
{
public:
	GameScene();
	~GameScene();

	void Initialize();

	void Update();

	void Draw();

private:

	DXCom* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Audio* audio_ = nullptr;

	Model* sphere = nullptr;
	Model* suzunne = nullptr;
	Model* fence = nullptr;

	std::vector<Model*> suzunnes;

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

};
