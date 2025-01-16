
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
	
}

///========================================================================================
///  敵の生成
///========================================================================================
void EnemyManager::SpawnEnemy(const std::string& enemyType, const Vector3& position) {

	std::unique_ptr<BaseEnemy> enemy;

	if (enemyType == enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]) { // 通常敵
		enemy = std::make_unique<NormalEnemy>();
		enemy->SetParm(BaseEnemy::Type::NORMAL,paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]);
	}

	else if (enemyType == enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)]) { // ストロングな敵
		enemy = std::make_unique<StrongEnemy>();
		enemy->SetParm(BaseEnemy::Type::STRONG,paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)]);
	}

	// 位置初期化とlistに追加
	enemy->Initialize();
	enemy->SetWorldPosition(Vector3(position.x, position.y, position.z));
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

		if ((*it)->GetType() == BaseEnemy::Type::NORMAL) {
			(*it)->SetParm(BaseEnemy::Type::NORMAL, paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]);
		}
		else if ((*it)->GetType() == BaseEnemy::Type::STRONG) {
			(*it)->SetParm(BaseEnemy::Type::STRONG, paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)]);
		}
		
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
void EnemyManager::Draw(Material* material) {
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
			paramaters_[i].fallSpeed);

		globalParameter_->AddItem(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].attackValue);

		globalParameter_->AddItem(
			groupName_,
			"Gravity" + std::to_string(int(i + 1)),
			paramaters_[i].gravityForKik);

		globalParameter_->AddItem(
			groupName_,
			"WeakJump" + std::to_string(int(i + 1)),
			paramaters_[i].jumpSpeed[0]);

		globalParameter_->AddItem(
			groupName_,
			"baseBoundPower_" + std::to_string(int(i + 1)),
			paramaters_[i].boundPower);

		globalParameter_->AddItem(
			groupName_,
			"attenuate_" + std::to_string(int(i + 1)),
			paramaters_[i].attenuate);

		globalParameter_->AddItem(
			groupName_,
			"gravityForBound" + std::to_string(int(i + 1)),
			paramaters_[i].gravityForBound);

		globalParameter_->AddItem(
			groupName_,
			"maxFallSpeedForBound" + std::to_string(int(i + 1)),
			paramaters_[i].maxFallSpeedForBound);


		globalParameter_->AddItem(
			groupName_,
			"maxFallSpeedForKik" + std::to_string(int(i + 1)),
			paramaters_[i].maxFallSpeedForKik);
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
			paramaters_[i].fallSpeed);

		globalParameter_->SetValue(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].attackValue);

		globalParameter_->SetValue(
			groupName_,
			"Gravity" + std::to_string(int(i + 1)),
			paramaters_[i].gravityForKik);

		globalParameter_->SetValue(
			groupName_,
			"WeakJump" + std::to_string(int(i + 1)),
			paramaters_[i].jumpSpeed[0]);

		globalParameter_->SetValue(
			groupName_,
			"baseBoundPower_" + std::to_string(int(i + 1)),
			paramaters_[i].boundPower);

		globalParameter_->SetValue(
			groupName_,
			"attenuate_" + std::to_string(int(i + 1)),
			paramaters_[i].attenuate);

		globalParameter_->SetValue(
			groupName_,
			"gravityForBound" + std::to_string(int(i + 1)),
			paramaters_[i].gravityForBound);

		globalParameter_->SetValue(
			groupName_,
			"maxFallSpeedForBound" + std::to_string(int(i + 1)),
			paramaters_[i].maxFallSpeedForBound);


		globalParameter_->SetValue(
			groupName_,
			"maxFallSpeedForKik" + std::to_string(int(i + 1)),
			paramaters_[i].maxFallSpeedForKik);

		globalParameter_->SetValue(
			groupName_,
			"DeathCount" + std::to_string(int(i + 1)),
			paramaters_[i].deathCountMax);
	}

}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void EnemyManager::ApplyGlobalParameter() {
	/// パラメータ
	for (uint32_t i = 0; i < paramaters_.size(); ++i) {
		paramaters_[i].fallSpeed = globalParameter_->GetValue<float>(
			groupName_,
			"FallSpeed" + std::to_string(int(i + 1)));

		paramaters_[i].attackValue = globalParameter_->GetValue<float>(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)));

		paramaters_[i].gravityForKik = globalParameter_->GetValue<float>(
			groupName_,
			"Gravity" + std::to_string(int(i + 1)));

		paramaters_[i].jumpSpeed[0] = globalParameter_->GetValue<float>(
			groupName_,
			"WeakJump" + std::to_string(int(i + 1)));

		paramaters_[i].boundPower = globalParameter_->GetValue<float>(
			groupName_,
			"baseBoundPower_" + std::to_string(int(i + 1)));

		paramaters_[i].attenuate = globalParameter_->GetValue<float>(
			groupName_,
			"attenuate_" + std::to_string(int(i + 1)));

		paramaters_[i].deathCountMax = globalParameter_->GetValue<int>(
			groupName_,
			"DeathCount" + std::to_string(int(i + 1)));

		paramaters_[i].maxFallSpeedForBound = globalParameter_->GetValue<float>(
			groupName_,
			"maxFallSpeedForBound" + std::to_string(int(i + 1)));

		paramaters_[i].maxFallSpeedForKik = globalParameter_->GetValue<float>(
			groupName_,
			"maxFallSpeedForKik" + std::to_string(int(i + 1)));

		paramaters_[i].gravityForBound = globalParameter_->GetValue<float>(
			groupName_,
			"gravityForBound" + std::to_string(int(i + 1)));
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

		ImGui::SeparatorText(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].c_str());
		ImGui::PushID(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].c_str());

		ImGui::DragFloat("FallSpeed",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].fallSpeed,
			0.01f);

		ImGui::DragFloat("AttackValue",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].attackValue,
			0.01f);

		ImGui::DragFloat("GravityForKik",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].gravityForKik,
			0.01f);

		ImGui::DragFloat("GravityForBound",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].gravityForBound,
			0.01f);

		ImGui::DragFloat("MaxFallSpeedForKik",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].maxFallSpeedForKik,
			0.01f);

		ImGui::DragFloat("MaxFallSpeedForBound",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].maxFallSpeedForBound,
			0.01f);

		ImGui::DragFloat("WeakJumpSpeed",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].jumpSpeed[0],
			0.01f);

		ImGui::DragFloat("BaseBoundPower",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].boundPower,
			0.01f);

		ImGui::DragFloat("BoundAttenuate",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].attenuate,
			0.01f);

		ImGui::SliderInt("DeathCount",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].deathCountMax,0,10);

		ImGui::PopID();

		///---------------------------------------------------------
		/// ストロングな敵
		///----------------------------------------------------------

		ImGui::SeparatorText(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)].c_str());
		ImGui::PushID(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)].c_str());

		ImGui::DragFloat("FallSpeed",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].fallSpeed,
			0.01f);

		ImGui::DragFloat("AttackValue",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].attackValue,
			0.01f);

		ImGui::DragFloat("GravityForKik",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].gravityForKik,
			0.01f);

		ImGui::DragFloat("GravityForBound",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].gravityForBound,
			0.01f);

		ImGui::DragFloat("MaxFallSpeedForKik",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].maxFallSpeedForKik,
			0.01f);

		ImGui::DragFloat("MaxFallSpeedForBound",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].maxFallSpeedForBound,
			0.01f);

		ImGui::DragFloat("WeakJumpSpeed",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].jumpSpeed[0],
			0.01f);

		ImGui::DragFloat("BoundPower",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].boundPower,
			0.01f);

		ImGui::DragFloat("BoundAttenuate",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].attenuate,
			0.01f);

		ImGui::SliderInt("DeathCount",
			&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].deathCountMax, 0, 10);

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

