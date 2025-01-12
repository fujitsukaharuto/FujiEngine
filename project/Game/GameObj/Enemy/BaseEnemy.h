#pragma once

// function
#include "Easing.h"
// class
#include "OriginGameObject.h"
#include"Collider/AABBCollider.h"
#include"Behavior/BaseEnemyBehavior.h"
#include<array>

class Player;
class BaseEnemy : public OriginGameObject {
private:
	std::unique_ptr<AABBCollider>collider_;
	std::unique_ptr<AABBCollider>jumpCollider_;
public:
	enum class JumpPower {
		WEAK,
		NORMAL,
		STRONG,
	};
protected:

	/// other class
	Player* pPlayer_;
	bool isdeath_;
	float hp_;
	float HPMax_;
	Vector2 hpbarSize_;
	float damageParm_;

	Easing spawnEasing_;

	/// parm
	float fallSpeed_;
	float attackValue_;
	float gravity_;

	JumpPower jumpPower_;
	std::array<float,1>jumpSpeed_;

	/// behavior
	std::unique_ptr<BaseEnemyBehaivor>behavior_ = nullptr;

public:
	static float InitY_;
	static Vector3 InitScale_;
public:
	BaseEnemy();

	///========================================================================================
	///  public method
	///========================================================================================

	/// 初期化、更新、描画
	virtual void Initialize()override;
	virtual void Update()override;
	virtual void Draw(Material* mate = nullptr)override;

	///* ジャンプ
	void Jump(float& speed);
	void Fall(float& speed, const bool& isJump = false);

	void SetParm(const float& fallSpeed, const float& attackValue, const float& gravity, const std::array<float, 1>& jumpSpeed);
	void DamageForPar(const float& par);
	void ChangeBehavior(std::unique_ptr<BaseEnemyBehaivor>behavior);

	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	BaseCollider* GetCollider() { return collider_.get(); } 
	BaseCollider* GetJumpCollider() { return jumpCollider_.get(); }
	///========================================================================================
	///  getter method
	///========================================================================================
	bool GetIsDeath()const { return isdeath_; }
	Player* GetPlayer() { return pPlayer_; }
	float GetFallSpeed()const { return fallSpeed_; }
	float GetAttackValue()const { return attackValue_; }
	float GetGravity()const { return gravity_; }
	JumpPower GetJumpPower()const { return jumpPower_; }
	BaseEnemyBehaivor* GetBehavior()const { return behavior_.get(); }
	 float GetJumpSpeed(JumpPower num) const { return jumpSpeed_[static_cast<size_t>(num)]; }
	///========================================================================================
	///  setter method
	///========================================================================================
	void SetPlayer(Player* plyaer);
	/*void SetPosition(const Vector3& spownPos) {
		model_->transform.translate = spownPos; 
	};*/

	void SetAttackCollisionSize(const Vector3& size);
};
