#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Camera/FollowCamera.h"
#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"
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

private:

	void InitGameObj();

	void GameOverUpdate();
	void ContinueUpdate();
	void PadSwitch();

	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<Boss> boss_ = nullptr;
	std::unique_ptr<Graphics::FollowCamera> followCamera_;

	// 絵は配置データ(Level)が持つ。ここは Initialize で名前から引いた参照
	Graphics::PlacedSprite* key_ = nullptr;
	bool isPadDraw_ = false;
	Graphics::PlacedSprite* pad_ = nullptr;

	Graphics::PlacedSprite* gameOver_ = nullptr;
	Graphics::PlacedSprite* gameOverSelector_ = nullptr;
	int selectPoint_ = 0;
	Math::Vector2 selectPointL_ = { 180.0f,450.0f };
	Math::Vector2 selectPointR_ = { 810.0f,450.0f };

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
	/// <remarks>シーン遷移の暗転は SceneManager が持っているのでここには無い。ゲームオーバーの絵より手前に出すため配置データの最後に置いてある</remarks>
	Graphics::PlacedSprite* flash_ = nullptr;
};
