#pragma once
#include "BaseScene.h"


class ResultScene :public BaseScene {
public:
	ResultScene();
	~ResultScene();

	void Initialize()override;

	void Update()override;

	void Draw()override;

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	float ins = 1.0f;

	Object3d* text = nullptr;
	float defoSize = 1.0f;
	float moveTime = 0.0f;

	Object3d* sky = nullptr;
	Object3d* boss = nullptr;
	std::vector<Object3d*> chorus;
	float chorusTime = 0.0f;
	float amplitude = 0.15f;
	float frequency = 0.01f;

};
