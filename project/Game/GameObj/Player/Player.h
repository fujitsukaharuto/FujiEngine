#pragma once


#include <memory>
#include"Vector3.h"

/// behavior
#include"GameObj/PlayerBehavior/BasePlayerBehavior.h"
#include"Game/OriginGameObject.h"

/// <summary>
/// プレイヤークラス
/// </summary>
class Boss;
class Player :public OriginGameObject {

private:
	/// other class
	Boss* pBoss_;
private:
	/// ===================================================
	///private variaus
	/// ===================================================

		///* behavior
	std::unique_ptr<BasePlayerBehavior>behavior_ = nullptr;

	///* 目標角度
	float objectiveAngle_;
	///* 速度
	Vector3 velocity_;
	Vector3 prePos_;

	///* 攻撃フラグ
	bool isAttack_;

	///* ダメージ
	bool isDamage_;
	float damageTime_;
	float damageCollTime_;
	float life_;

	float blinkTimer_;
	float blinkInterval_;
	bool isTransparent_;


	/// jump
	float jumpSpeed_;	/// ジャンプスピード
	float muzzelJumpSpeed_;	/// ジャンプ
	const float jumpLimit_ = -1.2f;

	// fall
	float fallSpeed_;

public:
	static float InitY_;
	Player();
	/// ===================================================
	///public method
	/// ===================================================

	//* 初期化、更新、
	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	///* 移動
	void Move(const float& speed);
	bool GetIsMoving();
	Vector3 GetInputVelocity();
	void MoveToLimit();


	void Jump(float& speed);
	void Fall();
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);

	void Debug()override;
	
	/// ダメージ
	void DamageRendition();
	void TakeDamage();

	/// ===================================================
	/// getter
	/// ===================================================
	const bool& GetIsAttack()const { return isAttack_; }
	Boss* GetBoss()const { return pBoss_; }
	float GetMuzzulJumpSpeed()const { return muzzelJumpSpeed_; }
	/// ===================================================
	/// setter
	/// ===================================================
	/*void SetJumpSpeed(float speed) { muzzelJumpSpeed_ = speed; }*/
	void SetIsAttack(bool is) { isAttack_ = is; }
};
