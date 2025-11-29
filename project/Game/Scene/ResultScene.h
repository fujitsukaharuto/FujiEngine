#pragma once
#include "Scene/BaseScene.h"
#include "Game/GameObj/SkyDome/SkyBox.h"

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

struct DanceStepTime {
	float turnLeftBaseTime = 25.0f;
	float turnRightBaseTime = 20.0f;
	float jumpLeftBaseTime = 20.0f;
	float jumpRightBaseTime = 20.0f;
	float jumpUpBaseTime = 25.0f;
	float fastSpinBaseTime = 20.0f;
	float finishSpinBaseTime = 30.0f;
	float lastBaseTime = 10.0f;
};

struct DanceStepHight {
	float jumpHeight = 1.0f;
	float spinHeight = 1.5f;
	float finishHeight = 2.2f;
	float lastHeight = 0.3f;
};

/// <summary>
/// リザルトシーンクラス
/// </summary>
class ResultScene :public BaseScene {
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

	std::unique_ptr<Object3dCommon> obj3dCommon = nullptr;
	std::unique_ptr<SkyBox> skybox_;
	std::unique_ptr<AnimationModel> terrain_ = nullptr;

	Vector3 rightDir = { 1.0f,0.0f,0.0f };

	std::unique_ptr<Sprite> clear_;

	std::vector<std::unique_ptr<Object3d>> players_;
	float xDiff_ = 5.0f;
	float zDiff_ = 2.0f;

	DanceState state_ = DanceState::TurnLeftMoveToLeft;
	DanceStepTime stepTime_;
	DanceStepHight hight_;
	float waitTime_ = 20.0f;
	float danceTime_ = 0.0f;
	float danceDistanceX_ = 3.5f;
	float jumpRotateZ_ = 0.4f;
	float lastRotateX_ = 0.1f;
	float lastRotateY_ = 0.2f;

	float defoTransY_ = 0.0f;
	float defoRotateY_ = 0.0f;

	float cameraStartRotateX_ = -0.1f;
	Vector3 lightDir_ = { 0.0f,-0.907f,0.42f };
	float lightIntens_ = 0.6f;

	// sceneChange
	std::unique_ptr<Sprite> black_;
	float blackLimmite = 20.0f;
	float blackTime = 20.0f;
	bool isChangeFase = false;
};
