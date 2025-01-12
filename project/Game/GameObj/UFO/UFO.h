#pragma once
#include"Vector3.h"

/// behavior
#include"Behavior/BaseUFOrBehavior.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"
#include"Collider/AABBCollider.h"

#include <memory>

class EnemyManager;
class UFO :public OriginGameObject {

private:
	/// other class
	EnemyManager* pEnemyManager_;
private:

	/// ===================================================
	///private variaus
	/// ===================================================

		///* behavior
	std::unique_ptr<BaseUFOBehavior>behavior_ = nullptr;
	std::unique_ptr<AABBCollider>collider_;
	
     /// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "UFO";      /// グループ名


	Vector3 popPos_;
	
	///parm
	float popWaitTime_;
	float dagameDistance_;
	float damageTime_;

public:
	static float InitY_;
	UFO();
	/// ===================================================
	///public method
	/// ===================================================

	//* 初期化、更新、描画
	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void EnemySpawn();
	void ChangeBehavior(std::unique_ptr<BaseUFOBehavior>behavior);

	/// ダメージ
	void DamageRendition();
	void TakeDamage();

	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	BaseCollider* GetCollider() { return collider_.get(); }
	
	///-------------------------------------------------------------------------------------
    /// Editor
    ///-------------------------------------------------------------------------------------
	void AdjustParm();
	void ParmLoadForImGui();
	void AddParmGroup();
	void SetValues();
	void ApplyGlobalParameter();


	/// ===================================================
	/// getter method
	/// ===================================================
	EnemyManager* GetEnemyManager() { return pEnemyManager_; }
	float GetPopWaitTime()const { return popWaitTime_; }
	float GetDamageDistance()const { return dagameDistance_; }
	float GetDamageTime()const { return damageTime_; }

	/// ===================================================
	/// setter method
	/// ===================================================
	void SetEnemyManager(EnemyManager*enemymanager);
};
