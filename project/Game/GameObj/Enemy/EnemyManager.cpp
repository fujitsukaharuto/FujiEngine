
#include "EnemyManager.h"
#include"NormalEnemy.h"
#include"StrongEnemy.h"

#include "FPSKeeper.h"


#include <format>
#include <fstream>
#include <imgui.h>

float EnemyManager::InitZPos_ = 0.0f;

EnemyManager::EnemyManager() {

}

///========================================================================================
///  初期化
///========================================================================================

void EnemyManager::Initialize() {
	

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();
	
	isEditorMode_ = false;
}

void  EnemyManager::FSpawn() {
	SpawnEnemy(enemyTypes_[static_cast<size_t>(Type::NORMAL)], Vector3{ -10, 30, InitZPos_ });
	SpawnEnemy(enemyTypes_[static_cast<size_t>(Type::STRONG)], Vector3{ 10, 30, InitZPos_ });
}

///========================================================================================
///  敵の生成
///========================================================================================
void EnemyManager::SpawnEnemy(const std::string& enemyType, const Vector3& position) {
	
		std::unique_ptr<BaseEnemy> enemy;

		if (enemyType == enemyTypes_[static_cast<size_t>(Type::NORMAL)]) { // 通常敵
			enemy = std::make_unique<NormalEnemy>();
			enemy->SetParm(paramaters_[static_cast<size_t>(Type::NORMAL)].fallSpeed_,
				paramaters_[static_cast<size_t>(Type::NORMAL)].attackValue_,
				paramaters_[static_cast<size_t>(Type::NORMAL)].gravity_,
				paramaters_[static_cast<size_t>(Type::NORMAL)].jumpSpeed_);
		}

		else if (enemyType == enemyTypes_[static_cast<size_t>(Type::STRONG)]) { // ストロングな敵
			enemy = std::make_unique<StrongEnemy>();
			enemy->SetParm(paramaters_[static_cast<size_t>(Type::STRONG)].fallSpeed_,
				paramaters_[static_cast<size_t>(Type::STRONG)].attackValue_,
				paramaters_[static_cast<size_t>(Type::STRONG)].gravity_,
				paramaters_[static_cast<size_t>(Type::STRONG)].jumpSpeed_);
		}

		// 位置初期化とlistに追加
		enemy->Initialize();
		enemy->SetPosition(Vector3(position.x,position.y, position.z));
		enemy->SetPlayer(pPlayer_);// プレイヤーセット
		enemies_.push_back(std::move(enemy));
}


///========================================================================================
///  更新処理
///========================================================================================
void EnemyManager::Update() {
	if (isEditorMode_) {
		return; // エディタモード中は停止
	}

	//SpawnUpdate(); // スポーン更新

	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		(*it)->Update(); // 更新

		if ((*it)->GetIsDeath()) {
			it = enemies_.erase(it); // 削除して次の要素を指すイテレータを取得
			//UpdateEnemyClearedFlag(); // フラグを更新
		}
		else {
			++it; // 削除しない場合はイテレータを進める
		}
	}
}


///========================================================================================
///  描画処理
///========================================================================================
void EnemyManager::Draw(Material*material) {
	for (auto it = enemies_.begin(); it != enemies_.end(); ++it) {
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
void EnemyManager::SetEditorMode(bool isEditorMode) {
	isEditorMode_ = isEditorMode;
}

///=================================================================================
/// ロード
///=================================================================================
void EnemyManager::ParmLoadForImGui() {

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
void EnemyManager::AddParmGroup() {

	for (uint32_t i = 0; i < paramaters_.size(); ++i) {
		globalParameter_->AddItem(
			groupName_,
			"FallSpeed" + std::to_string(int(i + 1)),
			paramaters_[i].fallSpeed_);

		globalParameter_->AddItem(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].attackValue_);

		globalParameter_->AddItem(
			groupName_,
			"Gravity" + std::to_string(int(i + 1)),
			paramaters_[i].gravity_);

		globalParameter_->AddItem(
			groupName_,
			"NormalJump" + std::to_string(int(i + 1)),
			paramaters_[i].jumpSpeed_[0]);
	}

}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void EnemyManager::SetValues() {


	for (uint32_t i = 0; i < paramaters_.size(); ++i) {
		globalParameter_->SetValue(
			groupName_,
			"FallSpeed" + std::to_string(int(i + 1)),
			paramaters_[i].fallSpeed_);

		globalParameter_->SetValue(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].attackValue_);

		globalParameter_->SetValue(
			groupName_,
			"Gravity" + std::to_string(int(i + 1)),
			paramaters_[i].gravity_);

		globalParameter_->SetValue(
			groupName_,
			"NormalJump" + std::to_string(int(i + 1)),
			paramaters_[i].jumpSpeed_[0]);
	}

}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void EnemyManager::ApplyGlobalParameter() {
	/// パラメータ
	for (uint32_t i = 0; i < paramaters_.size(); ++i) {
		paramaters_[i].fallSpeed_ = globalParameter_->GetValue<float>(
			groupName_,
			"FallSpeed" + std::to_string(int(i + 1)));

		paramaters_[i].attackValue_ = globalParameter_->GetValue<float>(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)));

		paramaters_[i].gravity_ = globalParameter_->GetValue<float>(
			groupName_,
			"Gravity" + std::to_string(int(i + 1)));

		paramaters_[i].jumpSpeed_[0] = globalParameter_->GetValue<float>(
			groupName_,
			"NormalJump" + std::to_string(int(i + 1)));
	}

}

///=========================================================
/// パラメータ調整
///==========================================================
void EnemyManager::AdjustParm() {
	SetValues();
#ifdef _DEBUG

	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		///---------------------------------------------------------
		/// 通常敵
		///----------------------------------------------------------

		ImGui::SeparatorText(enemyTypes_[static_cast<size_t>(Type::NORMAL)].c_str());
		ImGui::PushID(enemyTypes_[static_cast<size_t>(Type::NORMAL)].c_str());

		ImGui::DragFloat("FallSpeed",
			&paramaters_[static_cast<size_t>(Type::NORMAL)].fallSpeed_,
			0.01f);

		ImGui::DragFloat("AttackValue",
			&paramaters_[static_cast<size_t>(Type::NORMAL)].attackValue_,
			0.01f);

		ImGui::DragFloat("Gravity",
			&paramaters_[static_cast<size_t>(Type::NORMAL)].gravity_,
			0.01f);

		ImGui::DragFloat("NormalJumpSpeed",
			&paramaters_[static_cast<size_t>(Type::NORMAL)].jumpSpeed_[0],
			0.01f);

		ImGui::PopID();

		///---------------------------------------------------------
		/// ストロングな敵
		///----------------------------------------------------------

		ImGui::SeparatorText(enemyTypes_[static_cast<size_t>(Type::STRONG)].c_str());
		ImGui::PushID(enemyTypes_[static_cast<size_t>(Type::STRONG)].c_str());

		ImGui::DragFloat("FallSpeed",
			&paramaters_[static_cast<size_t>(Type::STRONG)].fallSpeed_,
			0.01f);

		ImGui::DragFloat("AttackValue",
			&paramaters_[static_cast<size_t>(Type::STRONG)].attackValue_,
			0.01f);

		ImGui::DragFloat("Gravity",
			&paramaters_[static_cast<size_t>(Type::STRONG)].gravity_,
			0.01f);

		ImGui::DragFloat("NormalJumpSpeed",
			&paramaters_[static_cast<size_t>(Type::STRONG)].jumpSpeed_[0],
			0.01f);

		ImGui::PopID();
		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();

		ImGui::PopID();
	}

#endif // _DEBUG
}

void EnemyManager::SetPlayer(Player* player) {
	pPlayer_ = player;
}   
void EnemyManager::SetLockon(LockOn* lockOn) {
	pLockOn_ = lockOn;
}

