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
	
    ///* グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "UFO";         /// グループ名

	Vector3 popPos_;                              /// 生成座標
	bool isDeath_;                                /// 死んだか
	float moveDirection;                          /// 移動向き
	float moveRestrectionPos_;
	
	///* parm 
	float hp_;                                    /// HP
	float MaxHp_;
	float popWaitTime_;                           
	float dagameDistance_;
	float damageTime_;
	float damageValue_;

	Vector3 collisionSize_;

	Vector4 ufoLightColor_ = { 0.796f,0.796f,0.129f,0.5f };

	///* obj
	std::unique_ptr<Object3d> ufoLight_;

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
	void UFOLightDraw();

	void Move();
	void EnemySpawn();
	void ChangeBehavior(std::unique_ptr<BaseUFOBehavior>behavior);

	/// ダメージ
	void DamageRendition();
	void TakeDamageForPar(const float& par);

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
	bool GetIsDeath()const { return isDeath_; }
	float GetDamageValue()const { return damageValue_; }
	/// ===================================================
	/// setter method
	/// ===================================================
	void SetEnemyManager(EnemyManager*enemymanager);
	void SetColor(const Vector4& color);
	void SetCollisionSize(const Vector3& size);
};
