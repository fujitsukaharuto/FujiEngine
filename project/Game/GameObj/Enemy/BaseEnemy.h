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
	enum class Type {
		NORMAL,
		STRONG,
	};

	enum class JumpPower {
		WEAK,
		NORMAL,
		STRONG,
	};

	struct Paramater {
		float                fallSpeed;
		float                attackValue;
		float                gravity;
		float                baseBoundPower;
		float                attenuate;
		int                  deathCount;
		std::array<float, 1> jumpSpeed;
	};
protected:

	/// other class
	Player*              pPlayer_;
			             
	bool                 isdeath_;
	float                hp_;
	float                HPMax_;
	Vector2              hpbarSize_;
	Easing               spawnEasing_;

	/// parm
	Paramater            paramater_;
	float                damageParm_;
	JumpPower            jumpPower_;
	int                  deathCountMax_;
	Type                 type_;
	
	/// behavior
	std::unique_ptr<BaseEnemyBehaivor> behavior_ = nullptr;

public:
	static float   InitY_;
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

	void SetParm(const Type&type,const Paramater&paramater);

	void DamageForPar(const float& par);
	void ChangeBehavior(std::unique_ptr<BaseEnemyBehaivor>behavior);

	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	BaseCollider* GetCollider() { return collider_.get(); }
	BaseCollider* GetJumpCollider() { return jumpCollider_.get(); }
	///========================================================================================
	///  getter method
	///========================================================================================
	bool               GetIsDeath()const { return isdeath_; }
	Player*            GetPlayer() { return pPlayer_; }
	float              GetFallSpeed()const { return paramater_.fallSpeed; }
	float              GetAttackValue()const { return paramater_.attackValue; }
	float              GetGravity()const { return paramater_.gravity; }
	int                GetDeathCount()const { return paramater_.deathCount; }
	int                GetDeathCountMax()const { return deathCountMax_; }
	float              GetBoundPower()const;
	JumpPower          GetJumpPower()const { return jumpPower_; }
	BaseEnemyBehaivor* GetBehavior()const { return behavior_.get(); }
	float              GetJumpSpeed(JumpPower num) const { return paramater_.jumpSpeed[static_cast<size_t>(num)]; }
	Type               GetType()const { return type_; }
	///========================================================================================
	///  setter method
	///========================================================================================
	void SetPlayer(Player* plyaer);
	void SetIsCollision(const bool& is);
	void DecrementDeathCount() { paramater_.deathCount--; }
};
