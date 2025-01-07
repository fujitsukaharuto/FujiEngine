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


	BaseCollider* GetCollider() { return collider_.get(); }

	void ColliderInit();

	std::string name_;

#ifdef _DEBUG
	void Debug()override;
	void DrawCollider();
#endif // _DEBUG

private:

	float time_;
	float omega_;
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };

	std::unique_ptr<AABBCollider> collider_ = nullptr;
	bool isCollider_ = false;


	ParticleEmitter hitParticle1_;
	ParticleEmitter hitParticle2_;
	ParticleEmitter hitParticle3_;
	ParticleEmitter hitParticle4_;

};
