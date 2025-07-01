#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"



class UnderRing : public OriginGameObject {
public:
	UnderRing();
	~UnderRing() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();

	void InitRing(const Vector3& pos);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	void SetIsLive(bool is) { isLive_ = is; }

	BaseCollider* GetCollider() { return collider_.get(); }
	float GetLifeTime() { return lifeTime_; }
	bool GetIsLive() { return isLive_; }
	float GetRingRadMax() { return ringRadMax_; }
	float GetRingRadMin() { return ringRadMin_; }

private:

private:

	std::unique_ptr<Object3d> cylinder_;

	bool isLive_ = false;
	float lifeTime_ = 300.0f;
	float speed_;
	float ringSize_;

	float ringRadMax_;
	float ringRadMin_;

	float uvTransX_;

	std::unique_ptr<AABBCollider> collider_;

};
