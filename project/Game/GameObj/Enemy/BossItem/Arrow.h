#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"
#include "Engine/Audio/AudioPlayer.h"


/// <summary>
/// BossItem:Arrowクラス
/// </summary>
class Arrow : public OriginGameObject {
public:
	Arrow();
	~Arrow();

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();

	/// <summary>追尾攻撃の初期化</summary>
	void InitArrow(const Vector3& pos,float emitTime);
	/// <summary>追尾先の設定</summary>
	void TargetSetting(const Vector3& target);

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
	void InitRod(const Vector3& pos, float time);

	/// <summary>避雷針の出現</summary>
	void FlyTimeUpdate();
	/// <summary>避雷針の落下処理</summary>
	void FallTimeUpdate();
	/// <summary>避雷針の破壊処理</summary>
	void BrokeTimeUpdate();

	bool GetIsBroke();

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is);
	void SetEmitterNumber(int num) { emitterNumber_ = num; }
	void SetArrowMode() { isArrow_ = true; }

	//========================================================================*/
	//* Getter
	BaseCollider* GetCollider() { return collider_.get(); }
	bool GetIsLive() { return isLive_; }


private:

private:

	bool isArrow_ = false;
	bool isLive_ = false;
	Vector3 velocity_;

	float animationTime_ = 0.0f;
	float totalAnimationTime_ = 30.0f;
	float emitTime_ = 0.0f;
	float arrivalTime_ = 0.0f;
	float totalArrivalTime_ = 30.0f;

	Vector3 startP_;
	Vector3 midtermP_;
	Vector3 endP_;
	float controlHeight_ = 20.0f;

	std::unique_ptr<AABBCollider> collider_;
	std::unique_ptr<Object3d> arrivalWarningPotion_;

	// emitter
	ParticleEmitter spark1_;
	ParticleEmitter spark2_;
	ParticleEmitter spark3_;

	ParticleEmitter hit_;
	ParticleEmitter hitParticle_;
	ParticleEmitter hitExpand_;

	SoundData* throwSE_ = nullptr;

	// Gpu Particle
	int emitterNumber_ = -1;


	// Rod
	float flyTime_ = 0.0f;
	float fallTime_ = 0.0f;
	float maxFallTime_ = 10.0f;
	float brokeTime_ = 0.0f;
	float maxBrokeTime_ = 120.0f;

	bool isBroke_ = false;
	bool isLightNing_ = true;

};
