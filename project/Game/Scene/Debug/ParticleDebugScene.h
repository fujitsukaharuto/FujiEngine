#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Sprite/Sprite.h"

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

	/// <summary>
	/// シーンチェンジ
	/// </summary>
	void BlackFade();

private:

	std::unique_ptr<Graphics::SkyBox> skybox_;
	std::unique_ptr<Graphics::AnimationModel> terrain_ = nullptr;
	Math::Vector4 terrainColor_ = { 0.85f,0.15f,0.1f,1.0f };
	Math::Vector4 skyBoxColor_ = { 0.45f,0.25f,0.4f,1.0f };

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimit_ = 20.0f;
	float blackTime_ = 20.0f;
	bool isChangePhase_ = false;
};
