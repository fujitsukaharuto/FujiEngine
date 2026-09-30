#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Game/GameObj/Player/Player.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"

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

	/// <summary>入力を見てシーン遷移を始める</summary>
	void CheckSceneChange();

private:

	void TitleLoadPlayerPoint();
	void TitleSavePlayerPoint();

#ifdef _DEBUGMODE
	bool uiInvisible_ = false;
#endif // _DEBUG

	// 絵は配置データ(Level)が持つ。ここは Initialize で名前から引いた参照
	Graphics::PlacedSprite* title_ = nullptr;
	Graphics::PlacedSprite* space_ = nullptr;

	float startTime_ = 90.0f;
	float startMaxTime_ = 90.0f;
	std::unique_ptr<Player> player_;
	Math::Vector3 playerStart_;
	Math::Vector3 playerCenter_;
	Math::Vector3 playerEnd_;

	/// <summary>演出の始まりの向き。Initialize で配置データの環境から取る</summary>
	Math::Vector3 cameraStartRotate_;
	float cameraEndRotateX_ = 0.15f;

	float titleCanMoveTime_ = 30.0f;
	float titleStartX_ = -640.0f;
	float titleY_ = 250.0f;
	float titleEmdX_ = 640.0f;

	float towerRad_ = 200.0f;
	int towerDivision_ = 12;

	std::unique_ptr<Graphics::Object3d> particleTest_ = nullptr;
	float csEmitterMoveTime_;

};
