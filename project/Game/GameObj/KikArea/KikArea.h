#pragma once

#include"OriginGameObject.h"
#include"Collider/AABBCollider.h"
#include"GlobalVariables.h"

class KikArea:public OriginGameObject {
private:

    /// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	const std::string groupName_ = "KikArea";      /// グループ名

	///　エリアのコライダー
	std::unique_ptr<AABBCollider> weakArea_;
	std::unique_ptr<AABBCollider> normalArea_;
	std::unique_ptr<AABBCollider> maxPowerArea_;

	/// Pos
	Vector3 weakAreaPos_;
	Vector3 normalAreaPos_;
	Vector3 maxPowerAreaPos_;

	/// Scale
	Vector3 weakAreaScale_;
	Vector3 normalAreaScale_;
	Vector3 maxPowerAreaScale_;


public:

	KikArea();
	~KikArea();
	/// ===================================================
	///public method
	/// ===================================================

	//* 初期化、更新、
	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void SetAreaSize(AABBCollider* collider, const Vector3& size);

	/// collider
	BaseCollider* GetWeakAreaCollider() { return weakArea_.get(); }
	BaseCollider* GetNormalAreaCollider() { return normalArea_.get(); }
	BaseCollider* GetMaxPowerArea() { return maxPowerArea_.get(); }

	///-------------------------------------------------------------------------------------
	/// Editor
	///-------------------------------------------------------------------------------------

	void AdjustParm();
	void ParmLoadForImGui();
	void AddParmGroup();
	void SetValues();
	void ApplyGlobalParameter();
};
