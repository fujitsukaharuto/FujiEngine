#pragma once
#include "Audio.h"
#include "DXCom.h"
#include "Input.h"
#include "DebugCamera.h"
#include "Object3d.h"
#include "Sprite.h"
#include "PointLight.h"
#include "SpotLight.h"


class GameScene
{
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


	Object3d* sphere = nullptr;
	Object3d* suzunne = nullptr;
	Object3d* fence = nullptr;
	Object3d* terrain = nullptr;

	std::vector<Object3d*> suzunnes;


	Sprite* test = nullptr;
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	float spherePara = 10;
	Vector3 spherevec = { 0.0f,1.0f,0.0f };

	float fencePara = 10;
	Vector3 fencevec = { -2.0f,-1.0f,1.0f };

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

};
