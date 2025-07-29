#pragma once
#include "Engine/DX/DXCom.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Input.h"
#include "AudioPlayer.h"
#include "DebugCamera.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model/AnimationData/AnimationModel.h"
#include "Sprite.h"
#include "Particle/ParticleEmitter.h"


class SceneManager;

class BaseScene {
public:
	BaseScene();
	virtual ~BaseScene() = default;

public:

	virtual void Initialize();
	virtual void Update();
	virtual void Draw();

	void Init(DXCom* pDxcom, SceneManager* pSceneManager);
	virtual void LoadSceneLevelData(const std::string& name);

	virtual void DebugGUI();
	virtual void ParticleDebugGUI();
	virtual void ParticleGroupDebugGUI();

	void ChangeScene(const std::string& sceneName, float extraTime);

private:





protected:

	DXCom* dxcommon_;
	SceneManager* sceneManager_;
	Input* input_ = nullptr;
	AudioPlayer* audioPlayer_ = nullptr;
	nlohmann::json sceneData_;

private:




};
