#include"FieldBlockManager.h"

#include "FPSKeeper.h"

#include <format>
#include"ImGuiManager.h"


float FieldBlockManager::InitZPos_ = 0.0f;

FieldBlockManager::FieldBlockManager() {

}

///========================================================================================
///  初期化
///========================================================================================

void FieldBlockManager::Initialize() {

	FieldBlock::nextSerialNum_ = 0;

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

}

void  FieldBlockManager::FSpawn() {

}

///========================================================================================
///  敵の生成
///========================================================================================
void FieldBlockManager::AddFieldBlock() {

	std::unique_ptr<FieldBlock> fieldBlick;

	// 通常敵

	fieldBlick = std::make_unique<FieldBlock>();
	fieldBlick->SetParm(paramaters_);

	// 位置初期化とlistに追加
	fieldBlick->Initialize();
	fieldBlick->SetPlayer(pPlayer_);// プレイヤーセット
	fieldBlocks_.push_back(std::move(fieldBlick));
}


///========================================================================================
///  更新処理
///========================================================================================
void FieldBlockManager::Update() {

	for (auto it = fieldBlocks_.begin(); it != fieldBlocks_.end(); ) {

		(*it)->SetParm(paramaters_);
		(*it)->Update(); // 更新

		if ((*it)->GetIsBreak()) {
			it = fieldBlocks_.erase(it); /// 削除して次の要素を指すイテレータを取得
		}
		else {
			++it; /// 削除しない場合はイテレータを進める
		}
	}
}


///========================================================================================
///  描画処理
///========================================================================================
void FieldBlockManager::Draw(Material* material) {
	for (auto it = fieldBlocks_.begin(); it != fieldBlocks_.end(); ++it) {
		(*it)->Draw(material);
	}
}

/////========================================================================================
///// フェーズ変更
/////========================================================================================
//void EnemyManager::SetPhase(int phase) {
//	if (phases_.find(phase) != phases_.end()) {
//		currentPhase_ = phase;
//		currentTime_ = 0.0f;
//	}
//	else {
//		ImGui::Text("Invalid phase!");
//	}
//}

///========================================================================================
/// エディターモード変更
///========================================================================================


///=================================================================================
/// ロード
///=================================================================================
void FieldBlockManager::ParmLoadForImGui() {

	/// ロードボタン
	if (ImGui::Button(std::format("Load {}", groupName_).c_str())) {

		globalParameter_->LoadFile(groupName_);
		/// セーブ完了メッセージ
		ImGui::Text("Load Successful: %s", groupName_.c_str());
		ApplyGlobalParameter();
	}
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void FieldBlockManager::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "breakCount", paramaters_.breakCount);
	globalParameter_->AddItem(groupName_, "restoreTime_", paramaters_.restoreTime_);
	globalParameter_->AddItem(groupName_, "damageCollTime_", paramaters_.damageCollTime_);
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void FieldBlockManager::SetValues() {

	globalParameter_->SetValue(groupName_, "breakCount", paramaters_.breakCount);
	globalParameter_->SetValue(groupName_, "restoreTime_", paramaters_.restoreTime_);
	globalParameter_->SetValue(groupName_, "damageCollTime_", paramaters_.damageCollTime_);
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void FieldBlockManager::ApplyGlobalParameter() {
	/// パラメータ
	paramaters_.breakCount = globalParameter_->GetValue<int>(groupName_, "breakCount");
	paramaters_.restoreTime_ = globalParameter_->GetValue<float>(groupName_, "restoreTime_");
	paramaters_.damageCollTime_ = globalParameter_->GetValue<float>(groupName_, "damageCollTime_");
}

///=========================================================
/// パラメータ調整
///==========================================================
void FieldBlockManager::AdjustParm() {
	SetValues();
#ifdef _DEBUG

	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		///---------------------------------------------------------
		/// 通常敵
		///----------------------------------------------------------

		ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

		ImGui::SliderInt("何回で壊れるか", &paramaters_.breakCount,0,100);
		ImGui::DragFloat("復活までの時間(秒)", &paramaters_.restoreTime_, 0.01f);
		ImGui::DragFloat("ダメージクールタイム", &paramaters_.damageCollTime_, 0.01f);

		ImGuiManager::GetInstance()->UnSetFont();

		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
		ImGui::PopID();


#endif // _DEBUG
	}
	for (auto it = fieldBlocks_.begin(); it != fieldBlocks_.end(); ++it) {
		(*it)->AdjustParm();
	}
}
void FieldBlockManager::SetPlayer(Player* player) {
	pPlayer_ = player;
}
