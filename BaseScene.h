#pragma once
#include "DXCom.h"
#include "Input.h"
#include "Audio.h"
#include "DebugCamera.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Sprite.h"
#include "ParticleEmitter.h"



class BaseScene {
public:
	BaseScene();
	virtual ~BaseScene() = default;

public:

	virtual void Initialize();

	virtual void Update();

	virtual void Draw();

	virtual void SpriteDraw();

	void Init();


private:





protected:

	DXCom* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Audio* audio_ = nullptr;

	float startTime = 5.0f;
private:




};
