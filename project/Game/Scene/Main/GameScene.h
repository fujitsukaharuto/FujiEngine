#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Camera/FollowCamera.h"
#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Sprite/Sprite.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"

/// <summary>
/// ゲームシーンクラス
/// </summary>
class GameScene :public Scene::BaseScene {
public:
	GameScene();
	~GameScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;

	/// <summary>ボス撃破やゲームオーバーの選択でシーン遷移を始める</summary>
	void CheckSceneChange();
	/// <summary>
	/// JsonからObject作る
	/// </summary>
	void LoadSceneLevelData(const std::string& name)override;

private:

	void InitGameObj();

	void CollisionUpdate();
	void GameOverUpdate();
	void ContinueUpdate();
	void PadSwitch();

	std::unique_ptr<Collision::CollisionManager> cMane_;

	std::unique_ptr<Graphics::AnimationModel> terrain_ = nullptr;
	std::unique_ptr<Graphics::Object3d> surroundings_ = nullptr;
	std::unique_ptr<Graphics::SkyBox> skybox_;
	Math::Vector4 terrainColor_ = { 0.85f,0.15f,0.1f,1.0f };
	Math::Vector4 surroundingColor_ = { 0.775f,0.385f,0.355f,1.0f };
	Math::Vector4 skyBoxColor_ = { 0.45f,0.25f,0.4f,1.0f };

	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<Boss> boss_ = nullptr;
	std::unique_ptr<Graphics::FollowCamera> followCamera_;

	std::unique_ptr<Graphics::Sprite> key_ = nullptr;
	bool isPadDraw_ = false;
	std::unique_ptr<Graphics::Sprite> pad_ = nullptr;

	std::unique_ptr<Graphics::Sprite> gameOver_ = nullptr;
	std::unique_ptr<Graphics::Sprite> gameOverSelector_ = nullptr;
	int selectPoint_ = 0;
	Math::Vector3 selectPointL_ = { 180.0f,450.0f,0.0f };
	Math::Vector3 selectPointR_ = { 810.0f,450.0f,0.0f };

	Math::Vector3 summonCameraPos_ = { -25.0f,5.0f,-25.0f };
	float panSpeed_ = 0.01f;

	bool isDebugCameraMode_ = false;

	float startPlayerLandingTime_ = 300.0f;

	Graphics::ParticleEmitter field_;

	Audio::SoundData* bgm_;

	float gameOverFadeTime_ = 0.0f;
	float continueFadeTime_ = 0.0f;
	float fadeBaseTime_ = 30.0f;
	bool isGameOverFade_ = false;
	bool isContinueFade_ = false;
	bool isRestartOnce_ = false;
	bool isGameOver_ = false;
	/// <summary>ゲームオーバー・コンティニューの明滅に使う黒</summary>
	/// <remarks>シーン遷移の暗転は SceneManager が持っているのでここには無い</remarks>
	std::unique_ptr<Graphics::Sprite> flash_;
};
