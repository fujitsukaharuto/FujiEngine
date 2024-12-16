#pragma once
#include "Scene/BaseScene.h"
#include "Game/TestBaseObj.h"
#include "Game/Collider/CollisionManager.h"
#include "Game/GameObj/Command.h"
#include "Game/GameObj/InputHandler.h"
#include "Game/GameObj/Player/Player.h"


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
	InputHandler* inputHandler_ = nullptr;
	ICommand* iCommand_ = nullptr;
	Player* player_;

};
