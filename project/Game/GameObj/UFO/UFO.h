#pragma once
#include"Vector3.h"

/// behavior
#include"Behavior/BaseUFOrBehavior.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"

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

	
     /// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "UFO";      /// グループ名


	Vector3 popPos_;
	
	///parm
	float popWaitTime_;

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

	/// ===================================================
	/// setter method
	/// ===================================================
	void SetEnemyManager(EnemyManager*enemymanager);
};
