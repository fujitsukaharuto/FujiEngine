#pragma once
#include "Scene/BaseScene.h"
#include "Game/TestBaseObj.h"
#include "Game/Collider/CollisionManager.h"
#include "Game/GameObj/StageCommand.h"
#include "Game/GameObj/StageSceneInputHandler.h"
#include "Game/GameObj/Selector/Selector.h"
#include "Game/GameObj/Unit/Unit.h"
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

	StageSceneInputHandler* inputHandler_ = nullptr;
	IStageCommand* command_ = nullptr;

	std::list<IStageCommand*> commandHistory_;
	std::list<IStageCommand*>::iterator commandHistoryItr_;

	Selector* selector_;

};
