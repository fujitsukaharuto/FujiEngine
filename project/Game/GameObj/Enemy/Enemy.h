#pragma once
#include "Game/OriginGameObject.h"
#include "ImGuiManager/ImGuiManager.h"
#include <numbers>
#include "Game/Collider/BaseCollider.h"
#include "Game/Collider/AABBCollider.h"
#include "Model/Line3dDrawer.h"
#include "Particle/ParticleEmitter.h"

class Enemy : public OriginGameObject {
public:
	Enemy();
	~Enemy()override;

public:

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	void OnCollisionExit(const ColliderInfo& other);

	void TargetChase();

	void SetTargetPos(const Vector3& target) { targetPos_ = target; }

	bool IsLive() { return isLive_; }
	BaseCollider* GetCollider() { return collider_.get(); }
	Vector3 GetWorldPos() const { return model_->GetWorldPos(); }

	void ColliderInit();

	float CustomEasing(float t);

	std::string name_;

#ifdef _DEBUG
	void Debug()override;
	void DrawCollider();
#endif // _DEBUG

private:

	float kSpeed_ = 0.025f;
	float kKnockSpeed_ = 0.25f;

	float time_;
	float omega_;
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };


	float life_ = 40.0f;
	bool isLive_ = true;


	std::unique_ptr<Object3d> body_;

	std::unique_ptr<AABBCollider> collider_ = nullptr;
	bool isCollider_ = false;


	bool isDamage_ = false;
	bool isKnockBack_ = false;

	float damegeTime_ = 0.0f;
	float knockBackTime_ = 0.0f;


	Vector3 targetPos_{};
	Vector3 toTarget_;

	ParticleEmitter hitParticle1_;
	ParticleEmitter hitParticle2_;
	ParticleEmitter hitParticle3_;
	ParticleEmitter hitParticle4_;

};
