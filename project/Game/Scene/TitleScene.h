#pragma once
#include "Scene/BaseScene.h"
#include "Game/TestBaseObj.h"
#include "Game/Collider/CollisionManager.h"
#include"Titleobj/TitleAnimationManager.h"

class TitleScene:public BaseScene {
public:
	TitleScene();
	~TitleScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;

	void BlackFade();

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<Object3d> sphere = nullptr;
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	ParticleEmitter emit1;
	ParticleEmitter emit2;

	std::unique_ptr<TestBaseObj> test_;
	std::unique_ptr<TestBaseObj> test2_;
	std::unique_ptr<CollisionManager> cMane_;
	std::unique_ptr<TitleAnimationManager>titleAnimation_;

	std::unique_ptr<Sprite> titlePaneru_;

	SoundData bgm_;

	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
};
