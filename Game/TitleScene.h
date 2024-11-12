#pragma once
#include "Scene/BaseScene.h"


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
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	Object3d* nightSky = nullptr;

	std::unique_ptr<Sprite> titletex;
};
