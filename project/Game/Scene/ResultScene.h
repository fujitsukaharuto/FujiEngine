#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Engine/Model/SkyBox/SkyBox.h"

/// <summary>
/// カービィダンスっぽい動き
/// </summary>
enum class DanceState {
	TurnLeftMoveToLeft,     // 1
	TurnRightMoveToCenter,  // 2
	JumpLeft,               // 3
	JumpRight,              // 4
	JumpUPSpin,             // 5
	FastSpin,               // 6
	FinishSpin,             // 7
	LastPose,               // 8
	Finish,                 // 9
};

/// <summary>
/// ダンスのステップそれぞれの基本時間
/// </summary>
struct DanceStepTime {
	float turnLeftBaseTime = 25.0f;
	float turnRightBaseTime = 20.0f;
	float jumpLeftBaseTime = 20.0f;
	float jumpRightBaseTime = 20.0f;
	float jumpUpBaseTime = 25.0f;
	float fastSpinBaseTime = 20.0f;
	float finishSpinBaseTime = 30.0f;
	float lastBaseTime = 10.0f;
	float hanabiWaitTime = 30.0f;
};

/// <summary>
/// ダンス時の高さの基準
/// </summary>
struct DanceStepHight {
	float jumpHeight = 1.0f;
	float spinHeight = 1.5f;
	float finishHeight = 2.2f;
	float lastHeight = 0.3f;
};

/// <summary>
/// 花火を出現させる範囲、各軸の最小値と最大値
/// </summary>
struct HanabiPopReng {
	Math::Vector2 xMinMax = { -8.0f, 8.0f };
	Math::Vector2 yMinMax = {  5.5f,10.0f };
	Math::Vector2 zMinMax = {  4.5f,12.0f };
};

/// <summary>
/// リザルトシーンクラス
/// </summary>
class ResultScene :public Scene::BaseScene {
public:
	ResultScene();
	~ResultScene();

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

	void ApplyGlobalVariables();//値読み込みテスト用今度Objectクラス作って継承で使えるようにする

	void KirbyDance();

	void HanabiUpdate();

	std::unique_ptr<Graphics::SkyBox> skybox_;
	std::unique_ptr<Graphics::AnimationModel> terrain_ = nullptr;
	std::unique_ptr<Graphics::Object3d> surroundings_ = nullptr;
	Math::Vector4 terrainColor_ = { 0.85f,0.15f,0.1f,1.0f };
	Math::Vector4 surroundingColor_ = { 0.775f,0.385f,0.355f,1.0f };
	Math::Vector4 skyBoxColor_ = { 0.45f,0.25f,0.4f,1.0f };

	std::unique_ptr<Graphics::Sprite> clear_;

	std::vector<std::unique_ptr<Graphics::Object3d>> players_;
	float xDiff_ = 5.0f;
	float zDiff_ = 2.0f;

	DanceState state_ = DanceState::TurnLeftMoveToLeft;
	DanceStepTime stepTime_;
	DanceStepHight height_;
	float waitTime_ = 20.0f;
	float danceTime_ = 0.0f;
	float danceDistanceX_ = 3.5f;
	float jumpRotateZ_ = 0.4f;
	float lastRotateX_ = 0.1f;
	float lastRotateY_ = 0.2f;

	float defaTransY_ = 0.0f;
	float defaRotateY_ = 0.0f;

	float cameraStartRotateX_ = -0.1f;
	Math::Vector3 cameraPos_ = { 0.0f, 2.0f, -20.0f };
	Math::Vector3 lightDir_ = { 0.0f,-0.907f,0.42f };
	float lightIntensity_ = 0.6f;

	int hanabiIndex_ = 0;
	HanabiPopReng popPos_;

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimit_ = 20.0f;
	float blackTime_ = 20.0f;
	bool isChangePhase_ = false;
};
