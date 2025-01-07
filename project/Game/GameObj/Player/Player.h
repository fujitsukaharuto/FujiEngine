#pragma once
#include "Game/OriginGameObject.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Game/Collider/BaseCollider.h"
#include "Game/Collider/AABBCollider.h"
#include "Math/MatrixCalculation.h"
#include "Game/OriginState.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class LockOn;

class Player : public OriginGameObject {
public:
	Player();
	~Player()override;

	enum class PlayerBehavior {
		kDefult,
		kAttack,
		kDush,
		kJump,
	};

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void BehaviorRequest();
	void SetState(std::unique_ptr<OriginState> behaviorState) { state_ = std::move(behaviorState); }
	void SetBehaviorRequest(PlayerBehavior behavior) { behaviorRequest_ = behavior; }

	void OnCollisionEnter(const ColliderInfo& other);
	BaseCollider* GetCollider() { return collider_.get(); }
	void OnCollisionAttackEnter(const ColliderInfo& other);
	BaseCollider* GetColliderAttack() { return colliderAttack_.get(); }
	AABBCollider* GetAABBAttack() { return colliderAttack_.get(); }


	void SetVelocity(const Vector3& v) { velocity_ = v; }

	void SetIsAttack(bool is) { isAttack_ = is; }
	bool GetIsAttack()const { return isAttack_; }


	Object3d* GetBodyModel() { return body_.get(); }
	Object3d* GetWeaponModel() { return weapon_.get(); }
	Object3d* GetFireModel() { return firePlane_.get(); }
	Trans& GetBodyTrans() { return body_->transform; }

	void EmitJumpAttack();

	void SetLockOn(const LockOn* target) { lockOn_ = target; }
	bool GetLockOn();
	const LockOn* GetLockOnPtr() { return lockOn_; }

#ifdef _DEBUG
	void Debug()override;
	void DrawCollider() { collider_->DrawCollider(); }
	void DrawColliderAttack() { if (isAttack_) { colliderAttack_->DrawCollider(); } }

#endif // _DEBUG


	SoundData attackSound1;
	SoundData attackSound2;
	SoundData attackSound3;

	SoundData dushSound;
	SoundData jumpSound;


private:

	std::unique_ptr<Object3d> body_;
	std::unique_ptr<Object3d> weapon_;
	std::unique_ptr<Object3d> firePlane_ = nullptr;

	std::unique_ptr<AABBCollider> collider_ = nullptr;
	std::unique_ptr<AABBCollider> colliderAttack_ = nullptr;


	Vector3 velocity_{};

	std::unique_ptr<OriginState> state_;
	PlayerBehavior behavior_ = PlayerBehavior::kDefult;
	std::optional<PlayerBehavior> behaviorRequest_ = std::nullopt;

	bool isAttack_ = false;
	bool isAttack2_ = false;



	ParticleEmitter attackParticle_;
	ParticleEmitter attackParticle2_;

	ParticleEmitter attackParticle3_;
	ParticleEmitter attackParticle4_;
	ParticleEmitter attackParticle5_;

	const LockOn* lockOn_ = nullptr;


};
