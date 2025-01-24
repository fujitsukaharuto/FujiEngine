#pragma once

///behavior
#include"State/BaseFieldBlockState.h"
#include"GlobalVariables.h"
// class
#include "OriginGameObject.h"
#include"Collider/AABBCollider.h"
#include<array>

class Player;
class FieldBlock : public OriginGameObject {
	
public:
	// パラメータ
	struct Paramater {
		int   breakCount;
		float restoreTime_;
	};

protected:

	/// other class
	Player*            pPlayer_;


	/// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	std::string groupName_;

			           
	/// parm		
	const std::string& tag_ = "FieldBlock";
	Paramater          paramater_;     //パラメータ
	bool               isBreak_;
	uint32_t           serialNum_;
	Vector3            collisionSize_;
	
	/// behavior
	std::unique_ptr<BaseFieldBlockState> state_ = nullptr;
	std::unique_ptr<AABBCollider>collider_;
public:
	static uint32_t nextSerialNum_;
public:
	FieldBlock();

	///========================================================================================
	///  public method
	///========================================================================================

	/// 初期化、更新、描画
	virtual void Initialize() override;
	virtual void Update()     override;
	virtual void Draw  (Material* mate = nullptr)override;


	void Bound(float& speed);
	void SpawnFall(float& speed, const bool& isJump);

	void SetParm        (const Paramater&paramater);

	void ChangeBehavior (std::unique_ptr<BaseFieldBlockState>behavior);

	void          OnCollisionEnter (const ColliderInfo& other);
	void          OnCollisionStay  (const ColliderInfo& other);
	BaseCollider* GetCollider      () { return collider_.get(); }

	///-------------------------------------------------------------------------------------
    ///Editor
    ///-------------------------------------------------------------------------------------
	void ParmLoadForImGui();
	void AddParmGroup();
	void SetValues();
	void ApplyGlobalParameter();
	void AdjustParm();


	///========================================================================================
	///  getter method
	///========================================================================================
	Player*            GetPlayer()                         { return pPlayer_; }
	bool               GetIsBreak()const                   { return isBreak_; }
	BaseFieldBlockState* GetBehavior()const                { return state_.get(); }
	Paramater          GetParamater()const                 { return paramater_; }
	///========================================================================================
	///  setter method
	///========================================================================================
	void               SetPlayer(Player* plyaer);
	void               SetIsBreak(const bool& is) { isBreak_ = is; };
	void               SetCollisionSize(const Vector3& size);
};
