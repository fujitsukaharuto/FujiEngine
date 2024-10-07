#pragma once
#include "Audio.h"
#include "DXCom.h"
#include "Input.h"
#include "DebugCamera.h"
#include "Model.h"
#include "Object/Player.h"
#include "Object/Boss.h"

#include<memory>

class GameScene{
public:
	GameScene();
	~GameScene();

	void Initialize();

	void Update();

	void Draw();

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

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

	std::vector<Model*> playerModels_ {};
	std::vector<Model*> bossModels_ {};
	/*======================
		プレイヤー
	=======================*/
	std::unique_ptr<Player>player_ = nullptr;

	/*======================
		敵
	=======================*/
	//ボス
	std::unique_ptr<Boss> boss_ = nullptr;


	std::vector<Model*> suzunnes;


	float spherePara = 10;
	Vector3 spherevec = { 0.0f,1.0f,0.0f };

	float fencePara = 10;
	Vector3 fencevec = { -2.0f,-1.0f,1.0f };

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

};
