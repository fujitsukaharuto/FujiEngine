#pragma once
#include"Vector3.h"

/// behavior
#include"GameObj/PlayerBehavior/BasePlayerBehavior.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"

#include <memory>
#include"Collider/AABBCollider.h"


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
	
	
/// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "Player";      /// グループ名

	///* 速度
	Vector3 velocity_;
	Vector3 prePos_;

	///* 目標角度
	float objectiveAngle_;
	
	/// 移動、ジャンプ
	float moveSpeed_;
	float airMoveSpeed_;
	float jumpSpeed_;
	float gravity_;

	// fall
	float fallSpeed_;

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
	float GetMoveSpeed()const { return moveSpeed_; }
	float GetAirMoveSpeed()const { return airMoveSpeed_; }
	float GetJumpSpeed()const { return jumpSpeed_; }

	/// ===================================================
	/// setter method
	/// ===================================================

};
