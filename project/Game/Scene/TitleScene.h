#pragma once
#include "Scene/BaseScene.h"
#include "Game/TestBaseObj.h"
#include "Game/GameObj/SkyDome/SkyBox.h"
#include "Game/GameObj/Enemy/BossItem/Arrow.h"
#include "Game/GameObj/Player/Player.h"
#include "Game/Collider/CollisionManager.h"

class TitleScene:public BaseScene {
public:
	TitleScene();
	~TitleScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;

	void BlackFade();

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<SkyBox> skybox_;
	std::unique_ptr<AnimationModel> terrain_ = nullptr;

	std::unique_ptr<Sprite> title_;
	std::unique_ptr<Sprite> space_;

	float startTime_ = 90.0f;
	float startMaxTime_ = 90.0f;
	std::unique_ptr<Player> player_;
	Vector3 playerStart_;
	Vector3 playerCenter_;
	Vector3 playerEnd_;

	float cameraStartRotateX_ = -0.5f;
	float cameraEndRotateX_ = 0.15f;

	float titleCanMoveTime_ = 30.0f;
	float titleStartX_ = -640.0f;
	float titleEmdX_ = 640.0f;


	std::unique_ptr<Object3d> particleTest_ = nullptr;

	ParticleEmitter emit;

	float csEmitterMoveTime_;

	std::unique_ptr<CollisionManager> cMane_;

	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
	bool isParticleDebugScene_ = false;
};
