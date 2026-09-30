#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Engine/Graphics/Object/AnimationModel.h"

/// <summary>
/// パーティクルデバッグシーンクラス
/// </summary>
class ParticleDebugScene :public Scene::BaseScene {
public:
	ParticleDebugScene();
	~ParticleDebugScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;
	void ParticleGroupDebugGUI()override;

	/// <summary>入力を見てシーン遷移を始める</summary>
	void CheckSceneChange();
};
