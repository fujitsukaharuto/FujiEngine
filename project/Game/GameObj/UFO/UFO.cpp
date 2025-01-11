#include "UFO.h"
///behavior
#include"Behavior/UFORoot.h"
#include"Behavior/UFOPopEnemy.h"
///* imgui
#include<imgui.h> 


float UFO::InitY_ = 1.5f;

UFO::UFO() {}

/// ===================================================
///  初期化
/// ===================================================
void UFO::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::SetModel("UFO.obj");

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	/// 通常モードから
	ChangeBehavior(std::make_unique<UFORoot>(this));
}

/// ===================================================
///  更新処理
/// ===================================================
void UFO::Update() {
	/*prePos_ = GetWorldPosition();*/
	/// ダメージエフェクト
	DamageRendition();
	/// 振る舞い処理
	behavior_->Update();
	//　移動制限
	/*MoveToLimit();*/
	
	/// 更新
	//base::Update();
}

/// ===================================================
///  描画
/// ===================================================
void UFO::Draw(Material* material) {

	OriginGameObject::Draw(material);
}


/// ===================================================
///  ダメージ演出
/// ===================================================
void UFO::DamageRendition() {
	
}


///=========================================================
///　移動制限
///==========================================================
//void Player::MoveToLimit() {
//
//	// フィールドの中心とスケールを取得
//	Vector3 fieldCenter = { 0.0f, 0.0f, 0.0f }; // フィールド中心 
//	Vector3 fieldScale = Field::baseScale_;     // フィールドのスケール
//
//	// プレイヤーのスケールを考慮した半径
//	float radiusX = fieldScale.x - model_->transform.scale.x;
//	float radiusZ = fieldScale.z - model_->transform.scale.z;
//
//	// 現在位置が範囲内かチェック
//	bool insideX = std::abs(model_->transform.translate.x - fieldCenter.x) <= radiusX;
//	bool insideZ = std::abs(model_->transform.translate.z - fieldCenter.z) <= radiusZ;
//
//	///-----------------------------------------------------------
//	///範囲外なら戻す
//	///-----------------------------------------------------------
//
//	if (!insideX) {/// X座標
//		model_->transform.translate.x = std::clamp(
//			model_->transform.translate.x,
//			fieldCenter.x - radiusX,
//			fieldCenter.x + radiusX
//		);
//	}
//
//	if (!insideZ) {/// Z座標
//		model_->transform.translate.z = std::clamp(
//			model_->transform.translate.z,
//			fieldCenter.z - radiusZ,
//			fieldCenter.z + radiusZ
//		);
//	}
//
//	// 範囲外の反発処理
//	if (!insideX || !insideZ) {
//		Vector3 directionToCenter = (fieldCenter - model_->transform.translate).Normalize();
//		model_->transform.translate.x += directionToCenter.x * 0.1f; // 軽く押し戻す
//		model_->transform.translate.z += directionToCenter.z * 0.1f; // 軽く押し戻す
//	}
//}

///=========================================================
/// ダメージ受ける
///==========================================================
void UFO::TakeDamage() {
	
}

///=========================================================
/// 振る舞い切り替え
///==========================================================
void UFO::ChangeBehavior(std::unique_ptr<BaseUFOBehavior>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
}
///=========================================================
/// パラメータ調整
///==========================================================
void UFO::AdjustParm() {
	SetValues();
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Player")) {

		/// 位置
		ImGui::SeparatorText("Transform");
		ImGui::DragFloat3("Position", &model_->transform.translate.x, 0.1f);

		///　Floatのパラメータ
		ImGui::SeparatorText("FloatParamater");
		
		
		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
	}

#endif // _DEBUG
}
///=================================================================================
/// ロード
///=================================================================================
void UFO::ParmLoadForImGui() {

	// ロードボタン
	if (ImGui::Button(std::format("Load {}", groupName_).c_str())) {

		globalParameter_->LoadFile(groupName_);
		// セーブ完了メッセージ
		ImGui::Text("Load Successful: %s", groupName_.c_str());
		ApplyGlobalParameter();
	}
}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void UFO::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "Translate", model_->transform.translate);
	

}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void UFO::SetValues() {

	globalParameter_->SetValue(groupName_, "Translate", model_->transform.translate);
	
}

///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void UFO::ApplyGlobalParameter() {
	model_->transform.translate = globalParameter_->GetValue<Vector3>(groupName_, "Translate");
	
}
///=========================================================
/// Class Set
///==========================================================

