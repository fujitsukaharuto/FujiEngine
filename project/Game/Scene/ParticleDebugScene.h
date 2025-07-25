#pragma once
#include "Scene/BaseScene.h"
#include "Game/GameObj/SkyDome/SkyBox.h"


class ParticleDebugScene :public BaseScene {
public:
	ParticleDebugScene();
	~ParticleDebugScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;
	void ParticleGroupDebugGUI()override;

	void BlackFade();

private:

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<SkyBox> skybox_;

	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
};
