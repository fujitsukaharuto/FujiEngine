#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"


class PlayerBullet : public OriginGameObject {
public:
	PlayerBullet();
	~PlayerBullet() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

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

	//========================================================================*/
	//* Release
	void Release(float speed,float damage,const Vector3& velo);

	//========================================================================*/
	//* Getter
	bool GetIsLive() { return isLive_; }
	bool GetIsCharge() { return isCharge_; }
	float GetDamage() { return damage_; }
	BaseCollider* GetCollider() { return collider_.get(); }

private:

private:

	std::unique_ptr<AABBCollider> collider_;

	bool isLive_ = false;
	bool isCharge_ = false;

	Vector3 velocity_;
	float speed_;
	float damage_;

	float zRotate_ = 0.0f;

};
