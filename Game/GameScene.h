#pragma once
#include "Audio.h"
#include "DXCom.h"
#include "Input.h"
#include "DebugCamera.h"

//local
#include "Object/Player.h"
#include "Object/NoteEnemy.h"
#include "Object/Boss.h"
#include "Field/Field.h"
#include "Object/EnemyManager.h"

#include<memory>

class GameScene{
public:
	GameScene();
	~GameScene();

	void Initialize();

	void Update();

	void Draw();

private:

	/// <summary>
	/// 敵のポップデータ読み込み
	/// </summary>
	void LoadEnemyPopData();

	/// <summary>
	/// 敵のポップ
	/// </summary>
	void UpdateEnemyPopData();

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
	std::unique_ptr<EnemyManager> enemyManager_ = nullptr;

	/*======================
		フィールド(五線譜)
	=======================*/
	std::array<Model*, 5> fieldModels_;
	std::unique_ptr<Field> field_ = nullptr;

	std::vector<Model*> suzunnes;


	float spherePara = 10;
	Vector3 spherevec = {0.0f,1.0f,0.0f};

	float fencePara = 10;
	Vector3 fencevec = {-2.0f,-1.0f,1.0f};

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

};
