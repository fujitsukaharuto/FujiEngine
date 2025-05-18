#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Game/GameObj/Enemy/Behavior/BaseBossBehavior.h"

class Boss : public OriginGameObject {
public:
	Boss();
	~Boss() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BaseBossBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);


	BaseCollider* GetCollider() { return collider_.get(); }
	float GetAttackCooldown() { return attackCooldown_; }

private:

private:

	std::unique_ptr<BaseBossBehavior> behavior_ = nullptr;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;


	float attackCooldown_ = 0.0f;

};
