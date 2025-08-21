#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"



class Arrow : public OriginGameObject {
public:
	Arrow();
	~Arrow() = default;

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

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	void SetIsLive(bool is) { isLive_ = is; }

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
	float controlHeight_ = 15.0f;

	std::unique_ptr<AABBCollider> collider_;

	// emitter
	ParticleEmitter spark1_;
	ParticleEmitter spark2_;


};
