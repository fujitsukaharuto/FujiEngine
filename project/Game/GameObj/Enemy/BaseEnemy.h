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

	enum class Tag {
		FALL,
		BLOWINGWEAK,
		BLOWINGSTRONG,
	};
public:
	// パラメータ
	struct Paramater {
		float                spawnFallSpeed;           /// 落ちる速さ(発生)
		float                spawnBoundSpeed;
		float                spawnBoundGravity;
		float                spawnBoundGravityMax;
		float                fallSpeed;                /// 落ちる速さ
		float                attackValue;              /// 攻撃力
		float                explotionTime_;           ///　爆発時間
		float                explotionExtensionTime_;  ///  爆発延長時間
		std::array<float, 2> blowingPower;             ///  吹っ飛びスピード
	};

protected:

	/// other class
	Player*                            pPlayer_;
			                           
	bool                               isdeath_;
	Easing                             spawnEasing_;
							           
	/// parm				           
	Paramater                          paramater_;     //パラメータ
	Type                               type_;
	std::array<std::string, 3>         tags_;
	float                              explotionTime_; //爆発時間
	float                              blowDirection_;
	
	/// behavior
	std::unique_ptr<BaseEnemyBehaivor> behavior_ = nullptr;

public:
	static float                       InitY_;
	static Vector3                     InitScale_;
	static float                       BoundPosY_;
	static float                       StartZPos_;
public:
	BaseEnemy();

	///========================================================================================
	///  public method
	///========================================================================================

	/// 初期化、更新、描画
	virtual void Initialize() override;
	virtual void Update()     override;
	virtual void Draw  (Material* mate = nullptr)override;

	void Bound(float& speed);
	void SpawnFall(float& speed, const bool& isJump);

	void SetParm        (const Type&type,const Paramater&paramater);
	void WallRefrection ();

	void ChangeBehavior (std::unique_ptr<BaseEnemyBehaivor>behavior);

	void          OnCollisionEnter (const ColliderInfo& other);
	void          OnCollisionStay  (const ColliderInfo& other);
	BaseCollider* GetCollider      () { return collider_.get(); }

	///========================================================================================
	///  getter method
	///========================================================================================
	Player*            GetPlayer()                         { return pPlayer_; }
	bool               GetIsDeath()const                   { return isdeath_; }
	BaseEnemyBehaivor* GetBehavior()const                  { return behavior_.get(); }
	Type               GetType()const                      { return type_; }
	Paramater          GetParamater()const                 { return paramater_; }
	float              GetExplotionTime()const             { return explotionTime_; }
	float              GetBlowDirection()const             { return blowDirection_; }
	///========================================================================================
	///  setter method
	///========================================================================================
	void               SetPlayer(Player* plyaer);
	void               SetIsDeath(const bool& is) { isdeath_ = is; };
};
