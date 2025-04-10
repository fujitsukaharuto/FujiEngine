#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Game/GameObj/Player/Behavior/BasePlayerBehavior.h"



class StageBlock;

class Player : public OriginGameObject{
public:
	Player();
	~Player() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void DebugGUI()override;

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);

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
	//* Getter
	float GetMoveSpeed() { return moveSpeed_; }
	float GetJumpSpeed() { return jumpSpeed_; }
	float GetFallSpeed() { return fallSpeed_; }
	int GetGrabDir() { return dir_; }
	bool GetIsFall() { return isFall_; }
	BaseCollider* GetCollider() { return collider_.get(); }

	//========================================================================*/
	//* Setter
	void SetFallSpeed(float speed) { fallSpeed_ = speed; }
	void SetIsFall(bool is) { isFall_ = is; }

private:

	/**
	* @brief 値の比較
	* @param a 現在の値
	* @param b 最大値
	* @return (a < b) ? a : b
	*/
	float ComparNum(float a, float b);

private:

	std::unique_ptr<BasePlayerBehavior> behavior_ = nullptr;

	std::unique_ptr<AABBCollider> collider_;

	float moveSpeed_;
	float jumpSpeed_;
	float fallSpeed_;
	float gravity_;
	float maxFallSpeed_;
	bool isFall_;

	Vector3 velocity_;
	Vector3 inputDirection_;
	int dir_;

};
