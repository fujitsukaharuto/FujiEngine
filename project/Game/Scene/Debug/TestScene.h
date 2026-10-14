#pragma once
#include "Engine/Scene/BaseScene.h"
#include <vector>
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"

/// <summary>
/// Testシーンクラス
/// </summary>
/// <remarks>レイトレ影の確認用にライトを並べてある。置物は配置データ(Level/TEST.json)にある</remarks>
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
	/// <remarks>枠は最初の1回だけ取るので、呼び直しても増えない</remarks>
	void SetupLights();
	/// <summary>指定した種類のライトだけを点ける</summary>
	/// <remarks>影は他の光源が当たっていると差が見えないので、1種類だけにして確認する</remarks>
	void ApplyLightPreset(bool directional, bool point, bool spot);

	// このシーンが使うスポットライトの枠。配置データのライトと取り合わないよう、決め打ちせず取った番号を持つ
	int spotSlot_ = -1;

private:

};
