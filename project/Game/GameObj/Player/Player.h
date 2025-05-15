#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Game/GameObj/Player/Behavior/BasePlayerBehavior.h"
#include "Game/GameObj/Player/AttackBehavior/BasePlayerAttackBehavior.h"


class PlayerBullet;

class Player : public OriginGameObject{
public:
	Player();
	~Player() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void DebugGUI()override;
	void ParameterGUI();

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);
	void ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Move
	void Move(const float& speed);
	Vector3 GetInputDirection();
	bool GetIsMove();

	//========================================================================*/
	//* Jump
	void Jump(float& speed);
	void Fall(float& speed);

	//========================================================================*/
	//* Bullet
	void InitBullet();
	void ReleaseBullet();
	void StrngthBullet();

	//========================================================================*/
	//* Getter
	float GetMoveSpeed() { return moveSpeed_; }
	float GetJumpSpeed() { return jumpSpeed_; }
	float GetFallSpeed() { return fallSpeed_; }
	float GetMaxChargeTime() { return maxChargeTime_; }
	int GetGrabDir() { return dir_; }
	bool GetIsFall() { return isFall_; }
	BaseCollider* GetCollider() { return collider_.get(); }

	PlayerBullet* GetPlayerBullet() { return bullet_.get(); }

	//========================================================================*/
	//* Setter
	void SetFallSpeed(float speed) { fallSpeed_ = speed; }
	void SetIsFall(bool is) { isFall_ = is; }
	void SetTargetPos(const Vector3& pos) { targetPos_ = pos; }

private:



private:

	std::unique_ptr<BasePlayerBehavior> behavior_ = nullptr;
	std::unique_ptr<BasePlayerAttackBehavior> attackBehavior_ = nullptr;
	std::unique_ptr<PlayerBullet> bullet_ = nullptr;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;

	Vector3 targetPos_;

	float moveSpeed_;
	float jumpSpeed_;
	float fallSpeed_;
	float gravity_;
	float maxFallSpeed_;
	float maxChargeTime_;
	bool isFall_;

	Vector3 velocity_;
	Vector3 inputDirection_;
	int dir_;

};
