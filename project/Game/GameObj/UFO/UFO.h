#pragma once
#include"Vector3.h"

/// behavior
#include"Behavior/BaseUFOrBehavior.h"
#include"State/BaseUFOState.h"

#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"
#include"Collider/AABBCollider.h"
#include "Model/Sprite.h"

#include <memory>

class EnemyManager;
class UFO :public OriginGameObject {
public:

	struct Paramater {
		Vector3 collisionSize_;
		Vector3 initLightScale;
		Vector3 lightScaleUnderPop;
		Vector3 startPos;
		Vector3 gamePos;
		Vector2 barPos_;
		Vector2 hpPos_;
		float dagameDistance_;
		float damageTime_;
		float hitStopDamage_;
		float hitStopTime_;
	};

private:
	/// other class
	EnemyManager* pEnemyManager_;
private:

	/// ===================================================
	///private variaus
	/// ===================================================

	///* behavior
	std::unique_ptr<BaseUFOBehavior>behavior_ = nullptr;
	std::unique_ptr<BaseUFOState>   state_ = nullptr;
	std::unique_ptr<AABBCollider>   collider_;
	
    ///* グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "UFO";         /// グループ名

	Vector3 popPos_;                              /// 生成座標
	bool isDeath_;                                /// 死んだか
	float moveDirection;                          /// 移動向き
	float moveRestrectionPos_;
	Vector3 hitPosition_;

	///* parm 
	float hp_;                                    /// HP
	float MaxHp_;
	float takeDamageValue_;

	Paramater paramater_;

	Vector4 ufoLightColor_ = { 0.796f,0.796f,0.129f,0.5f };

	///* obj
	std::unique_ptr<Object3d> ufoLight_;

	std::unique_ptr<Sprite> hpSprite_;
	float hpSize_ = 500.0f;
	float hpMaxSize_ = 500.0f;
	std::unique_ptr<Sprite> hpMaxSprite_;

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
	void SpriteDraw();

	void Move();
	void EnemySpawn();
	void ChangeBehavior(std::unique_ptr<BaseUFOBehavior>behavior);
	void ChangeState(std::unique_ptr<BaseUFOState>behavior);
	void ChangePopBehavior();

	/// ダメージ
	void DamageRendition();
	void TakeDamageForPar(const float& par);

	/// 出現
	void Apear(const float& time, const float& maxTime);

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
	Object3d* GetUFOLight() { return ufoLight_.get(); }
	Paramater GetParamater()const { return paramater_; }
	bool GetIsDeath()const { return isDeath_; }
	float GetDamageValue()const { return takeDamageValue_; }
	Vector3 GetHitPosition()const { return hitPosition_; }
	/// ===================================================
	/// setter method
	/// ===================================================
	void SetEnemyManager(EnemyManager*enemymanager);
	void SetColor(const Vector4& color);
	void SetCollisionSize(const Vector3& size);
};
