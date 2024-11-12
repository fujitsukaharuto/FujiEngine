#pragma once
#include "Scene/BaseScene.h"


#include "RailEditor.h"
#include "Player.h"
#include "Enemy.h"
#include "EnemyManager.h"

class GameScene :public BaseScene
{
public:
	GameScene();
	~GameScene();

	void Initialize()override;

	void Update()override;

	void Draw()override;

private:

	bool IsLineCollisionSphere(const Vector3& P1, const Vector3& P2, const Vector3& C, float r);

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;

	Object3d* nightSky = nullptr;
	Object3d* nightWater = nullptr;
	Object3d* terrain = nullptr;

	//std::vector<Object3d*> suzunnes;

	std::unique_ptr<Object3d> kurukuru = nullptr;
	float hitCheack = 0.0f;


	Sprite* scoreArea = nullptr;

	std::unique_ptr<Sprite> texScore[4];

	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

	ParticleEmitter emit;

	RailEditor editor;



	Player* player_;
	EnemyManager* enemyManager_;

	int score = 0;

};
