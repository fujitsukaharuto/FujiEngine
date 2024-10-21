#pragma once
#include "BaseScene.h"

//local
#include "Object/Player.h"
#include "Object/NoteEnemy.h"
#include "Object/Boss.h"
#include "Field/Field.h"
#include "Object/EnemyManager.h"

#include<memory>


class GameScene :public BaseScene
{
public:
	GameScene();
	~GameScene();

	void Initialize()override;

	void Update()override;

	void Draw()override;

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

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<Camera> camera = nullptr;

	Object3d* sphere = nullptr;
	Object3d* suzunne = nullptr;
	Object3d* fence = nullptr;
	Object3d* terrain = nullptr;

	/*======================
		カメラ
	=======================*/
	float cameraMoveSpeed_ = 0.01f;


	/*======================
		3dモデル
	=======================*/
	//Model* sphere = nullptr;
	//Model* suzunne = nullptr;
	//Model* fence = nullptr;
	//Model* ground = nullptr;

	std::vector<Object3d*> playerModels_ {};
	std::vector<Object3d*> bossModels_ {};
	/*======================
		プレイヤー
	=======================*/
	std::unique_ptr<Player>player_ = nullptr;

	std::vector<Object3d*> suzunnes;
	/*======================
		敵
	=======================*/
	//ボス
	std::unique_ptr<Boss> boss_ = nullptr;
	std::unique_ptr<EnemyManager> enemyManager_ = nullptr;

	/*======================
		フィールド(五線譜)
	=======================*/
	std::array<Object3d*, 5> fieldModels_;
	std::unique_ptr<Field> field_ = nullptr;


	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	float spherePara = 10;
	Vector3 spherevec = {0.0f,1.0f,0.0f};

	float fencePara = 10;
	Vector3 fencevec = {-2.0f,-1.0f,1.0f};

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

	ParticleEmitter emit;

};
