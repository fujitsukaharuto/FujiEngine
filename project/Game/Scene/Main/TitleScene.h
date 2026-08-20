#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Game/GameObj/Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Sprite/Sprite.h"

/// <summary>
/// タイトルシーンクラス
/// </summary>
class TitleScene:public Scene::BaseScene {
public:
	TitleScene();
	~TitleScene();

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

	void ApplyGlobalVariables();
	
	void TitleLoadPlayerPoint();
	void TitleSavePlayerPoint();

#ifdef _DEBUGMODE
	bool uiInvisible_ = false;
#endif // _DEBUG

	std::unique_ptr<Graphics::SkyBox> skybox_;
	std::unique_ptr<Graphics::AnimationModel> terrain_ = nullptr;
	std::unique_ptr<Graphics::Object3d> surroundings_ = nullptr;
	Math::Vector4 terrainColor_ = { 0.85f,0.15f,0.1f,1.0f };
	Math::Vector4 surroundingColor_ = { 0.775f,0.385f,0.355f,1.0f };
	Math::Vector4 skyBoxColor_ = { 0.45f,0.25f,0.4f,1.0f };

	std::unique_ptr<Graphics::Sprite> title_;
	std::unique_ptr<Graphics::Sprite> space_;

	float startTime_ = 90.0f;
	float startMaxTime_ = 90.0f;
	std::unique_ptr<Player> player_;
	Math::Vector3 playerStart_;
	Math::Vector3 playerCenter_;
	Math::Vector3 playerEnd_;

	float cameraStartRotateX_ = -0.5f;
	float cameraEndRotateX_ = 0.15f;
	Math::Vector3 cameraPos_ = { 0.0f, 5.0f, -30.0f };

	float titleCanMoveTime_ = 30.0f;
	float titleStartX_ = -640.0f;
	float titleY_ = 250.0f;
	float titleEmdX_ = 640.0f;
	Math::Vector2 titleSize_ = { 968.0f,159.0f };

	Math::Vector3 spacePos_ = { 640.0f,500.0f,0.0f };
	Math::Vector2 spaceSize_ = { 256.0f,128.0f };

	float towerRad_ = 200.0f;
	int towerDivision_ = 12;

	std::unique_ptr<Graphics::Object3d> particleTest_ = nullptr;
	float csEmitterMoveTime_;

	std::unique_ptr<Collision::CollisionManager> cMane_;

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimit_ = 20.0f;
	float blackTime_ = 20.0f;
	bool isChangePhase_ = false;
	bool isParticleDebugScene_ = false;
};
