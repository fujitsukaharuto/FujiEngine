#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Game/Collider/CollisionManager.h"

/// <summary>
/// Testシーンクラス
/// </summary>
class TestScene :public Scene::BaseScene {
public:
	TestScene();
	~TestScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;

	/// <summary>
	/// シーンチェンジ
	/// </summary>
	void BlackFade();

private:

	std::unique_ptr<CollisionManager> cMane_;

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimit_ = 20.0f;
	float blackTime_ = 20.0f;
	bool isChangePhase_ = false;
	bool isParticleDebugScene_ = false;
};
