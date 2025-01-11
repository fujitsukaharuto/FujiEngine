#pragma once

// function
#include "Easing.h"
// class
#include "OriginGameObject.h"
#include"Collider/AABBCollider.h"
#include"Behavior/BaseEnemyBehavior.h"

class Player;
class BaseEnemy : public OriginGameObject {
protected:

	/// other class
	Player* pPlayer_;
	bool isdeath_;
	float hp_;
	float HPMax_;
	Vector2 hpbarSize_;
	float damageParm_;

	Easing spawnEasing_;

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
	
	void DamageForPar(const float& par);

	void ChangeBehavior(std::unique_ptr<BaseEnemyBehaivor>behavior);


	///========================================================================================
	///  getter method
	///========================================================================================
	bool GetIsDeath()const { return isdeath_; }
	Player* GetPlayer() { return pPlayer_; }
	
	///========================================================================================
	///  setter method
	///========================================================================================
	void SetPlayer(Player* plyaer);
	void SetPosition(const Vector3& spownPos) {
		model_->transform.translate = spownPos; 
	};
};
