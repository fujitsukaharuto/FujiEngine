#pragma once
#include "Scene/BaseScene.h"
#include"GameObj/Timer/Timer.h"
#include"Game/reslutobj/ResultAnimationManager.h"

class ResultScene :public BaseScene {
public:
	ResultScene();
	~ResultScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;

	void BlackFade();

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<Object3d> sphere = nullptr;
	Vector3 rightDir = { 1.0f,0.0f,0.0f };


	std::unique_ptr<Sprite> clearPaneru_;
	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;

	std::unique_ptr<Timer>timer_;
	std::unique_ptr<ResultAnimationManager>reani_;
};
