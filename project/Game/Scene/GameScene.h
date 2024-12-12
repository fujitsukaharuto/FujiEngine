#pragma once
#include "Scene/BaseScene.h"


class GameScene :public BaseScene
{
public:
	GameScene();
	~GameScene();

	void Initialize()override;

	void Update()override;

	void Draw()override;

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;

};
