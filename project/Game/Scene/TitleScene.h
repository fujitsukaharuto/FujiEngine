#pragma once
#include "Scene/BaseScene.h"
#include "Game/TestBaseObj.h"
#include "Game/Collider/CollisionManager.h"
#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/FollowCamera.h"

class TitleScene:public BaseScene {
public:
	TitleScene();
	~TitleScene();

	void Initialize()override;

	void Update()override;

	void Draw()override;

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	/*std::unique_ptr<Object3d> sphere = nullptr;
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	ParticleEmitter emit;

	std::unique_ptr<TestBaseObj> test_;
	std::unique_ptr<TestBaseObj> test2_;*/
	std::unique_ptr<CollisionManager> cMane_;

	std::unique_ptr<Object3d> terrain = nullptr;
	std::unique_ptr<Player> player_;
	std::unique_ptr<FollowCamera> followCamera_;

};
