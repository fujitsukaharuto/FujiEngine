#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Engine/Particle/ParticleEmitter.h"

class PlayerBullet : public OriginGameObject {
public:
	PlayerBullet();
	~PlayerBullet();

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;

	void InitParameter(const Vector3& pos);
	void CalculetionFollowVec(const Vector3& target);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Charge
	void Charge(const Vector3& pos, const Vector3& rot);
	void StrnghtBullet();

	//========================================================================*/
	//* Release
	void Release(float speed,float damage,const Vector3& velo);

	//========================================================================*/
	//* Getter
	bool GetIsLive() { return isLive_; }
	bool GetIsCharge() { return isCharge_; }
	bool GetIsStrnght() { return isStrnght_; }
	float GetDamage() { return damage_; }
	BaseCollider* GetCollider() { return collider_.get(); }

	void SetIsLive(bool is) { isLive_ = is; }

private:

private:

	std::unique_ptr<AABBCollider> collider_;

	ParticleEmitter trajectory;
	ParticleEmitter trajectory2;

	ParticleEmitter hit_;
	ParticleEmitter hit2_;
	ParticleEmitter hit3_;
	ParticleEmitter hitSmoke_;
	ParticleEmitter hitcircle_;;


	bool isLive_ = false;
	bool isCharge_ = false;
	bool isStrnght_ = false;

	Vector3 velocity_;
	float speed_;
	float damage_;

	float zRotate_ = 0.0f;

};
