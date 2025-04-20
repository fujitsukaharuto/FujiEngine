#pragma once
#include "Engine/DX/DXCom.h"
#include "Input.h"
#include "AudioPlayer.h"
#include "DebugCamera.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model/AnimationData/AnimationModel.h"
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

	void Init(DXCom* pDxcom);

	virtual void DebugGUI();
	virtual void ParticleDebugGUI();
	virtual void ParticleGroupDebugGUI();

private:





protected:

	DXCom* dxcommon_;
	Input* input_ = nullptr;
	AudioPlayer* audioPlayer_ = nullptr;


private:




};
