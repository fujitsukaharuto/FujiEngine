#pragma once
#include "Audio.h"
#include "DXCom.h"
#include "Input.h"
#include "DebugCamera.h"
#include "Model.h"

#include<memory>

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

	/*======================
		3dモデル
	=======================*/
	Model* sphere = nullptr;
	Model* suzunne = nullptr;
	Model* fence = nullptr;

	Model* ground = nullptr;

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

};
