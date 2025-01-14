#pragma once
#include"Vector3.h"
#include"Collider/AABBCollider.h"
/// behavior
#include"GameObj/PlayerBehavior/BasePlayerBehavior.h"
#include"GameObj/PlayerBehavior/BasePlayerAttackBehavior.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"

#include <memory>

/// <summary>
/// プレイヤークラス
/// </summary>
class Player :public OriginGameObject {

private:
	/// other class
	/*Boss* pBoss_;*/
private:

	/// ===================================================
	///private variaus
	/// ===================================================
	
		///* behavior
	std::unique_ptr<BasePlayerBehavior>behavior_ = nullptr;
	std::unique_ptr<BasePlayerAttackBehavior>attackBehavior_ = nullptr;

/// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "Player";      /// グループ名

	///* 速度
	Vector3 velocity_;
	Vector3 prePos_;

	///* 目標角度
	float objectiveAngle_;
	Vector3 inputDirection_;
	
	/// 移動、ジャンプ
	float moveSpeed_;
	float airMoveSpeed_;
	float jumpSpeed_;
	float gravity_;
	float recoilSpeed_;
	float recoilJumpSpeed_;
	float kikTime_;

	// fall
	float fallSpeed_;

	std::unique_ptr<AABBCollider>weakikCollider_;

public:
	static float InitY_;
	Player();
	/// ===================================================
	///public method
	/// ===================================================

	//* 初期化、更新、描画
	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	///* 移動
	void Move(const float& speed);
	bool GetIsMoving();
	Vector3 GetInputVelocity();
	void MoveToLimit();

	///* ジャンプ
	void Jump(float& speed);
	void Fall(float& speed,const bool&isJump=false);
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);
	void ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior>behavior);


	/// ダメージ
	void DamageRendition();
	void TakeDamage();

	
	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	BaseCollider* GetWeakColliderCollider() { return weakikCollider_.get(); }
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
	float GetMoveSpeed()const { return moveSpeed_; }
	float GetAirMoveSpeed()const { return airMoveSpeed_; }
	float GetJumpSpeed()const { return jumpSpeed_; }
	float GetRecoilJumpSpeed()const { return recoilJumpSpeed_; }
	float  GetRecoilSpeed()const { return recoilSpeed_; }
	float  GetKikTime()const { return kikTime_; }
	float GetFacingDirection()const;
	/// ===================================================
	/// setter method
	/// ===================================================
	void SetIsSetCollision(const bool& is);
};
