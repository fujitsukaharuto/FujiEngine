#pragma once
#include "Scene/BaseScene.h"
#include "Engine/Model/AnimationData/AnimationModel.h"
#include "Game/GameObj/SkyDome/SkyBox.h"
#include "Game/GameObj/FollowCamera.h"
#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/OriginObject/TestBaseObj.h"
#include "Game/Collider/CollisionManager.h"

/// <summary>
/// ゲームシーンクラス
/// </summary>
class GameScene :public BaseScene {
public:
	GameScene();
	~GameScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;

	/// <summary>
	/// シーンチェンジ
	/// </summary>
	void BlackFade();
	/// <summary>
	/// JsonからObject作る
	/// </summary>
	void LoadSceneLevelData(const std::string& name)override;

private:

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	void GameoverUpdate();
	void ContinueUpdate();
	void PadSwitch();

	std::unique_ptr<Graphics::Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<CollisionManager> cMane_;

	std::unique_ptr<Graphics::AnimationModel> terrain_ = nullptr;
	std::unique_ptr<SkyBox> skybox_;
	Math::Vector4 terrainColor_ = { 0.85f,0.15f,0.1f,1.0f };
	Math::Vector4 skyBoxColor_ = { 0.45f,0.25f,0.4f,1.0f };

	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<Boss> boss_ = nullptr;
	std::unique_ptr<FollowCamera> followCamera_;

	std::unique_ptr<Graphics::Sprite> key_ = nullptr;
	bool isPadDraw_ = false;
	std::unique_ptr<Graphics::Sprite> pad_ = nullptr;

	std::unique_ptr<Graphics::Sprite> gameover_ = nullptr;
	bool isBackTitle_ = false;
	std::unique_ptr<Graphics::Sprite> gameoverSelector_ = nullptr;
	int selectPoint_ = 0;
	Math::Vector3 selectPointL_ = { 180.0f,450.0f,0.0f };
	Math::Vector3 selectPointR_ = { 810.0f,450.0f,0.0f };

	Math::Vector3 summonCameraPos = { -25.0f,5.0f,-25.0f };
	float panSpeed_ = 0.01f;

	bool isDebugCameraMode_ = false;

	float startPlayerLandingTime_ = 300.0f;

	ParticleEmitter emit;
	ParticleEmitter field;

	SoundData* bgm_;

	float gameoverFadeTime_ = 0.0f;
	float contiuneFadeTime_ = 0.0f;
	float fadeBaseTime_ = 30.0f;
	bool isGameoverFade_ = false;
	bool isContiuneFade_ = false;
	bool isRestartOnce_ = false;
	bool isGameover_ = false;

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimmite_ = 20.0f;
	float blackTime_ = 20.0f;
	bool isChangeFase_ = false;
};
