#pragma once
#include "Scene/BaseScene.h"

///* object
#include"GameObj/Player/Player.h"
#include"GameObj/Field/Field.h"
#include"GameObj/SkyDome/SkyDome.h"
#include"GameObj/GameCamera/GameCamera.h"
#include"GameObj/Enemy/EnemyManager.h"
#include"GameObj/UFO/UFO.h"
#include"Collider/CollisionManager.h"

class GameScene :public BaseScene
{
public:
	GameScene();
	~GameScene();

	/// ===================================================
	///pulic method
	/// ===================================================

	void Initialize()override;
	void Update()override;
	void Draw()override;

	void BlackFade();

	void ParamaterEdit();
private:

	/// ===================================================
	///private variaus
	/// ===================================================

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;

	std::unique_ptr<Object3d> sphere = nullptr;
	std::unique_ptr<Object3d> suzunne = nullptr;
	std::unique_ptr<Object3d> fence = nullptr;
	std::unique_ptr<Object3d> terrain = nullptr;
	

	std::vector<std::unique_ptr<Object3d>> suzunnes;

	std::unique_ptr<Material> mate = nullptr;

	std::unique_ptr<Sprite> test = nullptr;
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	float spherePara = 10;
	Vector3 spherevec = { 0.0f,1.0f,0.0f };

	float fencePara = 10;
	Vector3 fencevec = { -2.0f,-1.0f,1.0f };

	bool isDebugCameraMode_ = false;


	SoundData soundData1;
	SoundData soundData2;

	ParticleEmitter emit;


	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
	///* obj
	std::unique_ptr<Player>player_;
	std::unique_ptr<Field>field_;
	std::unique_ptr<SkyDome>skydome_;
	std::unique_ptr<GameCamera>gameCamera_;
	std::unique_ptr<EnemyManager>enemyManager_;
	std::unique_ptr<UFO>ufo_;

	std::unique_ptr<CollisionManager> cMane_;
};
