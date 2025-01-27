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
#include"GameObj/KikArea/KikArea.h"
#include"GameObj/Enemy/EnemySpawn/EnemySpawn.h"
#include"GameObj/FieldBlock/FieldBlockManager.h"
#include"GameObj/Timer/Timer.h"

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
	void Menu();
	void MenuInit();

	void ParamaterEdit();
private:

	void StartUI();

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

	Vector4 colorUihome = { 0.0f,0.0f,0.0f,1.0f };
	std::unique_ptr<Sprite> keyPaneru_;

	std::unique_ptr<Sprite> startUI_;
	Vector3 startUIPos_ = { 1020.0f,0.0f,0.0f };
	float slideTime_ = 0.0f;
	float slideLimitteTime_ = 5.0f;
	std::unique_ptr<Sprite> menuPaneru_;
	std::unique_ptr<Sprite> menuButton1_;
	std::unique_ptr<Sprite> menuButton2_;
	bool isMenu_ = false;
	int nowSelect_ = 1;
	Vector2 buttonSize1_ = { 300.0f,100.0f };
	Vector2 buttonSize2_ = { 300.0f,100.0f };
	Vector2 buttonSizeMax_ = { 420.0f,140.0f };
	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
	bool isTitle_ = false;
	///* obj
	std::unique_ptr<Player>player_;
	std::unique_ptr<Field>field_;
	std::unique_ptr<SkyDome>skydome_;
	std::unique_ptr<GameCamera>gameCamera_;
	std::unique_ptr<EnemyManager>enemyManager_;
	std::unique_ptr<UFO>ufo_;
	std::unique_ptr<KikArea>kikArea_;
	std::unique_ptr<EnemySpawn>enemySpawn_;
	std::unique_ptr<FieldBlockManager>fieldBlockManager_;
	std::unique_ptr<Timer>timer_;

	std::unique_ptr<CollisionManager> cMane_;
};
