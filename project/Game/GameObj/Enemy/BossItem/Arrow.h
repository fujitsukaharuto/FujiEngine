#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"



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

	void InitParameter();

	void InitArrow(const Vector3& pos,float emitTime);
	void TargetSetting(const Vector3& target);

	void EmitTimeUpdate();
	void AnimaTimeUpdate();
	void ArrivalTimeUpdate();

	void GPUEmitterSetting();

	// lightningRod
	void RodUpdate();
	void InitRod(const Vector3& pos, float time);

	void FlyTimeUpdate();
	void FallTimeUpdate();
	void BrokeTimeUpdate();

	bool GetIsBroke();

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	void SetIsLive(bool is);
	void SetEmitterNumber(int num) { emitterNumber_ = num; }

	BaseCollider* GetCollider() { return collider_.get(); }
	bool GetIsLive() { return isLive_; }


private:

private:

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

	// emitter
	ParticleEmitter spark1_;
	ParticleEmitter spark2_;
	ParticleEmitter spark3_;

	// Gpu Particle
	int emitterNumber_;


	// Rod
	float flyTime_ = 0.0f;
	float fallTime_ = 0.0f;
	float maxFallTime_ = 10.0f;
	float brokeTime_ = 0.0f;
	float maxBrokeTime_ = 120.0f;

	bool isBroke_ = false;
	bool isLightNing_ = true;

};
