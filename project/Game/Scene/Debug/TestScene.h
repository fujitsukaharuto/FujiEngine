#pragma once
#include "Engine/Scene/BaseScene.h"
#include "Game/Collider/CollisionManager.h"
#include <vector>

/// <summary>
/// Testシーンクラス
/// </summary>
/// <remarks>レイトレ影の確認用にオブジェクトとライトを並べてある。Player/Boss は使わない</remarks>
class TestScene :public Scene::BaseScene {
public:
	TestScene();
	~TestScene();

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

	/// <summary>地面と、影を落とすオブジェクトを並べる</summary>
	void SetupObjects();
	/// <summary>平行光源/点光源/スポットライトを1つずつ置く</summary>
	/// <remarks>本数ごと設定するので、シーンに入り直しても増えない</remarks>
	void SetupLights();
	/// <summary>ライトを起動直後の状態へ戻す</summary>
	/// <remarks>ここで置いたライトを他のシーンへ持ち越さないため</remarks>
	void RestoreLights();
	/// <summary>指定した種類のライトだけを点ける</summary>
	/// <remarks>影は他の光源が当たっていると差が見えないので、1種類だけにして確認する</remarks>
	void ApplyLightPreset(bool directional, bool point, bool spot);

private:

	std::unique_ptr<CollisionManager> cMane_;

	std::unique_ptr<Graphics::Object3d> ground_;
	std::vector<std::unique_ptr<Graphics::Object3d>> objects_;
	// スキンメッシュ。影を受ける側と落とす側を兼ねる
	std::vector<std::unique_ptr<Graphics::AnimationModel>> animeObjects_;

	// sceneChange
	std::unique_ptr<Graphics::Sprite> black_;
	float blackLimit_ = 20.0f;
	float blackTime_ = 20.0f;
	bool isChangePhase_ = false;
	bool isParticleDebugScene_ = false;
};
