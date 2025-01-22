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
	
public:
	enum class Type {
		NORMAL,
		STRONG,
	};

	enum class BlowingPower {
		WEAK,
		MaxPower,
	};

	struct Paramater {
		float                fallSpeed;
		float                attackValue;
		std::array<float, 2> blowingPower;
	};

protected:

	/// other class
	Player*              pPlayer_;
			             
	bool                 isdeath_;
	Easing               spawnEasing_;

	/// parm
	Paramater            paramater_;
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

	void SetParm(const Type&type,const Paramater&paramater);

	void ChangeBehavior(std::unique_ptr<BaseEnemyBehaivor>behavior);

	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	BaseCollider* GetCollider() { return collider_.get(); }

	///========================================================================================
	///  getter method
	///========================================================================================
	Player*            GetPlayer()                         { return pPlayer_; }
	bool               GetIsDeath()const                   { return isdeath_; }
	BaseEnemyBehaivor* GetBehavior()const                  { return behavior_.get(); }
	Type               GetType()const                      { return type_; }
	Paramater          GetParamater()const                 { return paramater_; }
	///========================================================================================
	///  setter method
	///========================================================================================
	void               SetPlayer(Player* plyaer);
};
