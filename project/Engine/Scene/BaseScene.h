#pragma once
#include "DXCom.h"
#include "Input.h"
#include "AudioPlayer.h"
#include "DebugCamera.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Sprite.h"
#include "Particle/ParticleEmitter.h"



class BaseScene {
public:
	BaseScene();
	virtual ~BaseScene() = default;

public:

	virtual void Initialize();

	virtual void Update();

	virtual void Draw();

	void Init();


private:





protected:

	DXCom* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	AudioPlayer* audioPlayer_ = nullptr;


private:




};
