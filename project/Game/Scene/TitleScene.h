#pragma once
#include "Scene/BaseScene.h"
#include "Game/TestBaseObj.h"
#include "Game/Collider/CollisionManager.h"

class TitleScene:public BaseScene {
public:
	TitleScene();
	~TitleScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;

	void BlackFade();

private:

	void SphereUpdate();

	std::pair<Vector3,Vector3> ComputeCollisionVelocity(float m1, const Vector3& v1, float m2, const Vector3& v2, float ref, const Vector3& normal);

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<Object3d> sphere1 = nullptr;
	std::unique_ptr<Object3d> sphere2 = nullptr;
	Vector3 rightDir = { 1.0f,0.0f,0.0f };


	/// sphere1
	Vector3 velocity1_{};
	float mass1 = 1.0f;

	/// sphere2
	Vector3 velocity2_{};
	float mass2 = 2.0f;

	float restituion = 0.5f;


	std::unique_ptr<CollisionManager> cMane_;


	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
};
