#pragma once
#include "Scene/BaseScene.h"
#include "Game/GameObj/SkyDome/SkyBox.h"

/// <summary>
/// パーティクルデバッグシーンクラス
/// </summary>
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

	/// <summary>
	/// シーンチェンジ
	/// </summary>
	void BlackFade();

private:

	std::unique_ptr<Graphics::Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<SkyBox> skybox_;
	std::unique_ptr<Graphics::AnimationModel> terrain_ = nullptr;
	Math::Vector4 terrainColor_ = { 0.85f,0.15f,0.1f,1.0f };
	Math::Vector4 skyBoxColor_ = { 0.45f,0.25f,0.4f,1.0f };

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
};
