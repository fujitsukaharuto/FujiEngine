#pragma once
#include "Scene/BaseScene.h"

///* object
#include"GameObj/Player/Player.h"
#include"Field/Field.h"

class GameScene :public BaseScene
{
public:
	GameScene();
	~GameScene();

	/// ===================================================
	///pulic method
	/// ===================================================

	void Initialize()override;
	void Update()override;
	void Draw()override;
private:

	/// ===================================================
	///private variaus
	/// ===================================================

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;

	///* obj
	std::unique_ptr<Player>player_;
	std::unique_ptr<Field>field_;
};
