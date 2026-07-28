#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Game/Particle/GameEmitters.h"


/// <summary>
/// BossItem:Arrowクラス
/// </summary>
class Arrow : public OriginGameObject {
public:
	Arrow();
	~Arrow();

	void Initialize()override;
	void Update()override;
	void Draw(bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();

	/// <summary>追尾攻撃の初期化</summary>
	void InitArrow(const Math::Vector3& pos,float emitTime);
	/// <summary>追尾先の設定</summary>
	void TargetSetting(const Math::Vector3& target);

	/// <summary>出現時の処理</summary>
	void EmitTimeUpdate();
	/// <summary>発射前処理</summary>
	void AnimaTimeUpdate();
	/// <summary>発射後追尾処理</summary>
	void ArrivalTimeUpdate();

	void GPUEmitterSetting();

	//========================================================================*/
	//* lightningRod
	/// <summary>避雷針の更新</summary>
	void RodUpdate();
	/// <summary>避雷針の初期化</summary>
	void InitRod(const Math::Vector3& pos, float time);

	/// <summary>避雷針の出現</summary>
	void FlyTimeUpdate();
	/// <summary>避雷針の落下処理</summary>
	void FallTimeUpdate();
	/// <summary>避雷針の破壊処理</summary>
	void BrokeTimeUpdate();

	bool GetIsBroke();

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other)override;
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other)override;
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other)override;

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is);
	void SetEmitterNumber(int num) { emitterNumber_ = num; }
	void SetArrowMode() { isArrow_ = true; }

	//========================================================================*/
	//* Getter
	bool GetIsLive() { return isLive_; }


private:

private:

	//========================================================================*/
	//* 定数
	static constexpr float kArrowModelScale_ = 5.0f;         // 矢モデルのスケール
	static constexpr float kThrowSEVolume_ = 0.2f;           // 発射SEの音量
	static constexpr float kArrivalRotationLerpRate_ = 0.1f; // 追尾中の回転補間レート
	static constexpr int   kHitEmitCount_ = 300;             // ヒット時GPUエミッターの生成数
	static constexpr float kHitEmitLifeTime_ = 0.5f;         // ヒット時GPUエミッターの寿命
	// Rod（避雷針）
	static constexpr float kRodFallStartHeight_ = 9.0f;      // 落下開始のY高さ
	static constexpr float kRodFallEndHeight_ = 1.0f;        // 落下終了(着地)のY高さ
	static constexpr float kLightningTriggerTime_ = 5.0f;    // 破壊直前に雷を出す残り時間
	static constexpr float kLightningSpawnHeight_ = 40.0f;   // 雷パーティクルの発生Y高さ

	bool isArrow_ = false;
	bool isLive_ = false;
	Math::Vector3 velocity_;

	float animationTime_ = 0.0f;
	float totalAnimationTime_ = 30.0f;
	float emitTime_ = 0.0f;
	float arrivalTime_ = 0.0f;
	float totalArrivalTime_ = 30.0f;

	Math::Vector3 startP_;
	Math::Vector3 midtermP_;
	Math::Vector3 endP_;
	float controlHeight_ = 20.0f;

	/// <summary>所有権は基底の colliders_ が持つ</summary>
	AABBCollider* collider_ = nullptr;
	/// <summary>着弾予告の円。所有権は基底の renderers_ が持つ</summary>
	Graphics::Object3d* arrivalWarningPotion_ = nullptr;

	// emitter
	Graphics::ParticleEmitter spark1_;
	Graphics::ParticleEmitter spark2_;
	Graphics::ParticleEmitter spark3_;

	Graphics::ParticleEmitter hit_;
	Graphics::ParticleEmitter hitParticle_;
	Graphics::ParticleEmitter hitExpand_;
	int hitEmitIndex_ = Game::kInvalidEmitterIndex;
	int hitEmitTrailIndex_ = Game::kInvalidEmitterIndex;

	Audio::SoundData* throwSE_ = nullptr;

	// Gpu Particle
	int emitterNumber_ = Game::kInvalidEmitterIndex;


	// Rod
	float flyTime_ = 0.0f;
	float fallTime_ = 0.0f;
	float maxFallTime_ = 10.0f;
	float brokeTime_ = 0.0f;
	float maxBrokeTime_ = 120.0f;

	bool isBroke_ = false;
	bool isLightNing_ = true;

};
