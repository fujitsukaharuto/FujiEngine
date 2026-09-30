#pragma once
#include "Engine/Scene/BaseScene.h"
#include <vector>
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"

/// <summary>
/// Testシーンクラス
/// </summary>
/// <remarks>レイトレ影の確認用にライトを並べてある。影を落とす置物は配置データ(Level/TEST.json)にある。Player/Boss は使わない</remarks>
class TestScene :public Scene::BaseScene {
public:
	TestScene();
	~TestScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;

	/// <summary>入力を見てシーン遷移を始める</summary>
	void CheckSceneChange();

private:

	/// <summary>点光源/スポットライトを1つずつ置く</summary>
	/// <remarks>平行光源は配置データの環境にある(影が足元に隠れないよう傾けてある)。
	/// 本数ごと設定するので、シーンに入り直しても増えない。シーンを出るとライトは Level が起動時の状態へ戻す</remarks>
	void SetupLights();
	/// <summary>指定した種類のライトだけを点ける</summary>
	/// <remarks>影は他の光源が当たっていると差が見えないので、1種類だけにして確認する</remarks>
	void ApplyLightPreset(bool directional, bool point, bool spot);

private:

};
