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

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<CollisionManager> cMane_;

	std::unique_ptr<AnimationModel> terrain = nullptr;
	std::unique_ptr<SkyBox> skybox_;
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<Boss> boss_ = nullptr;
	std::unique_ptr<FollowCamera> followCamera_;

	std::unique_ptr<Sprite> key_ = nullptr;

	std::unique_ptr<Sprite> gameover_ = nullptr;
	bool isBackTitle_ = false;
	std::unique_ptr<Sprite> gameoverSelector_ = nullptr;
	int selectPoint_ = 0;
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	Vector3 summonCameraPos = { -25.0f,5.0f,-25.0f };
	float panSpeed_ = 0.01f;

	bool isDebugCameraMode_ = false;

	float startPlayerLandingTime_ = 300.0f;

	ParticleEmitter emit;
	ParticleEmitter field;

	SoundData* bgm_;

	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
};
