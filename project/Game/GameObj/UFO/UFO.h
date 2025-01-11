#pragma once
#include"Vector3.h"

/// behavior
#include"Behavior/BaseUFOrBehavior.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"

#include <memory>

/// <summary>
/// プレイヤークラス
/// </summary>
class UFO :public OriginGameObject {

private:
	/// other class
	/*Boss* pBoss_;*/
private:

	/// ===================================================
	///private variaus
	/// ===================================================

		///* behavior
	std::unique_ptr<BaseUFOBehavior>behavior_ = nullptr;

	
/// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "UFO";      /// グループ名

	

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


	/// ===================================================
	/// setter method
	/// ===================================================

};
