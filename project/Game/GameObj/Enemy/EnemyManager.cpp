
#include "EnemyManager.h"
#include"NormalEnemy.h"
#include"StrongEnemy.h"

#include "FPSKeeper.h"

#include <format>
#include <fstream>
#include"ImGuiManager.h"
#include <imgui.h>

float EnemyManager::InitZPos_ = 0.0f;

EnemyManager::EnemyManager() {

}

///========================================================================================
///  初期化
///========================================================================================

void EnemyManager::Initialize() {
	enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)] = "NormalEnemy";
	enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)] = "StrongEnemy";

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	isEditorMode_ = false;
}

void  EnemyManager::FSpawn() {
	SpawnEnemy(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)], Vector3(-3, 60, BaseEnemy::StartZPos_));
	SpawnEnemy(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)], Vector3(0, 50, BaseEnemy::StartZPos_));
	SpawnEnemy(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)], Vector3(8, 70, BaseEnemy::StartZPos_));
}

///========================================================================================
///  敵の生成
///========================================================================================
void EnemyManager::SpawnEnemy(const std::string& enemyType, const Vector3& position) {

	std::unique_ptr<BaseEnemy> enemy;

	// 通常敵
	if (enemyType == enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]) {
		enemy = std::make_unique<NormalEnemy>();
		enemy->SetParm(BaseEnemy::Type::NORMAL, paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]);
	}

	// ストロングな敵
	else if (enemyType == enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)]) { 
		enemy = std::make_unique<StrongEnemy>();
		enemy->SetParm(BaseEnemy::Type::STRONG, paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)]);
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
			it = enemies_.erase(it); /// 削除して次の要素を指すイテレータを取得
			///UpdateEnemyClearedFlag(); // フラグを更新
		}
		else {
			++it; /// 削除しない場合はイテレータを進める
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
			"WeakBlow" + std::to_string(int(i + 1)),
			paramaters_[i].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::WEAK)]);

		globalParameter_->AddItem(
			groupName_,
			"MaxPowerBlow" + std::to_string(int(i + 1)),
			paramaters_[i].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::MaxPower)]);

		globalParameter_->AddItem(
			groupName_,
			"explotionTime_" + std::to_string(int(i + 1)),
			paramaters_[i].explotionTime_);


		globalParameter_->AddItem(
			groupName_,
			"explotionExtensionTime_" + std::to_string(int(i + 1)),
			paramaters_[i].explotionExtensionTime_);

		globalParameter_->AddItem(
			groupName_,
			"spawnFallSpeed" + std::to_string(int(i + 1)),
			paramaters_[i].spawnFallSpeed);

		globalParameter_->AddItem(
			groupName_,
			"spawnBoundSpeed" + std::to_string(int(i + 1)),
			paramaters_[i].spawnBoundSpeed);

		globalParameter_->AddItem(
			groupName_,
			"spawnBoundGravity" + std::to_string(int(i + 1)),
			paramaters_[i].spawnBoundGravity);

		globalParameter_->AddItem(
			groupName_,
			"spawnBoundGravityMax" + std::to_string(int(i + 1)),
			paramaters_[i].spawnBoundGravityMax);
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
			"WeakBlow" + std::to_string(int(i + 1)),
			paramaters_[i].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::WEAK)]);

		globalParameter_->SetValue(
			groupName_,
			"MaxPowerBlow" + std::to_string(int(i + 1)),
			paramaters_[i].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::MaxPower)]);

		globalParameter_->SetValue(
			groupName_,
			"explotionTime_" + std::to_string(int(i + 1)),
			paramaters_[i].explotionTime_);


		globalParameter_->SetValue(
			groupName_,
			"explotionExtensionTime_" + std::to_string(int(i + 1)),
			paramaters_[i].explotionExtensionTime_);

		globalParameter_->SetValue(
			groupName_,
			"spawnFallSpeed" + std::to_string(int(i + 1)),
			paramaters_[i].spawnFallSpeed);

		globalParameter_->SetValue(
			groupName_,
			"spawnBoundSpeed" + std::to_string(int(i + 1)),
			paramaters_[i].spawnBoundSpeed);

		globalParameter_->SetValue(
			groupName_,
			"spawnBoundGravity" + std::to_string(int(i + 1)),
			paramaters_[i].spawnBoundGravity);

		globalParameter_->SetValue(
			groupName_,
			"spawnBoundGravityMax" + std::to_string(int(i + 1)),
			paramaters_[i].spawnBoundGravityMax);
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


		paramaters_[i].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::WEAK)] = globalParameter_->GetValue<float>(
			groupName_,
			"WeakBlow" + std::to_string(int(i + 1)));


		paramaters_[i].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::MaxPower)] = globalParameter_->GetValue<float>(
			groupName_,
			"MaxPowerBlow" + std::to_string(int(i + 1)));

		paramaters_[i].explotionTime_ = globalParameter_->GetValue<float>(
			groupName_,
			"explotionTime_" + std::to_string(int(i + 1)));

		paramaters_[i].explotionExtensionTime_ = globalParameter_->GetValue<float>(
			groupName_,
			"explotionExtensionTime_" + std::to_string(int(i + 1)));

		paramaters_[i].spawnFallSpeed = globalParameter_->GetValue<float>(
			groupName_,
			"spawnFallSpeed" + std::to_string(int(i + 1)));

		paramaters_[i].spawnBoundSpeed = globalParameter_->GetValue<float>(
			groupName_,
			"spawnBoundSpeed" + std::to_string(int(i + 1)));

		paramaters_[i].spawnBoundGravity = globalParameter_->GetValue<float>(
			groupName_,
			"spawnBoundGravity" + std::to_string(int(i + 1)));

		paramaters_[i].spawnBoundGravityMax = globalParameter_->GetValue<float>(
			groupName_,
			"spawnBoundGravityMax" + std::to_string(int(i + 1)));

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

		if (ImGui::TreeNode(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].c_str())) {

			ImGui::PushID(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].c_str());

			ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

			ImGui::SeparatorText("出現時");

			ImGui::DragFloat("出現時の落ちる速さ(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].spawnFallSpeed,
				0.01f);

			ImGui::DragFloat("出現時UFOに当たった時のバウンド力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].spawnBoundSpeed,
				0.01f);

			ImGui::DragFloat("バウンド中の重力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].spawnBoundGravity,
				0.01f);

			ImGui::DragFloat("バウンド中の重力(最大)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].spawnBoundGravityMax,
				0.01f);


			ImGui::SeparatorText("出現後");
			ImGui::DragFloat("落ちる速さ(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].fallSpeed,
				0.01f);

			ImGui::DragFloat("攻撃力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].attackValue,
				0.01f);

			ImGui::DragFloat("弱いキックでの吹っ飛び(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::WEAK)],
				0.01f);


			ImGui::DragFloat("強いキックでの吹っ飛び(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::MaxPower)],
				0.01f);

			ImGui::DragFloat("爆発する時間(秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].explotionTime_,
				0.01f);

			ImGui::DragFloat("爆発する延長時間(秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].explotionExtensionTime_,
				0.01f);
			ImGuiManager::GetInstance()->UnSetFont();
			ImGui::PopID();

			ImGui::TreePop();
		}

			///---------------------------------------------------------
			/// ストロングな敵
			///----------------------------------------------------------

		if (ImGui::TreeNode(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)].c_str())) {
			ImGui::PushID(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)].c_str());

			ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

			ImGui::SeparatorText("出現時");

			ImGui::DragFloat("出現時の落ちる速さ(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].spawnFallSpeed,
				0.01f);

			ImGui::DragFloat("出現時UFOに当たった時のバウンド力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].spawnBoundSpeed,
				0.01f);

			ImGui::DragFloat("バウンド中の重力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].spawnBoundGravity,
				0.01f);

			ImGui::DragFloat("バウンド中の重力(最大)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].spawnBoundGravityMax,
				0.01f);


			ImGui::SeparatorText("出現後");
			ImGui::DragFloat("落ちる速さ(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].fallSpeed,
				0.01f);

			ImGui::DragFloat("攻撃力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].attackValue,
				0.01f);

			ImGui::DragFloat("弱いキックでの吹っ飛び(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::WEAK)],
				0.01f);

			ImGui::DragFloat("強いキックでの吹っ飛び(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::MaxPower)],
				0.01f);

			ImGui::DragFloat("爆発する時間(秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].explotionTime_,
				0.01f);

			ImGui::DragFloat("爆発する延長時間(秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].explotionExtensionTime_,
				0.01f);

			ImGuiManager::GetInstance()->UnSetFont();
			ImGui::PopID();

			ImGui::TreePop();
		}
			/// セーブとロード
			globalParameter_->ParmSaveForImGui(groupName_);
			ParmLoadForImGui();

			ImGui::PopID();
		}

#endif // _DEBUG
	}

	void EnemyManager::SetPlayer(Player * player) {
		pPlayer_ = player;
	}
	void EnemyManager::SetLockon(LockOn * lockOn) {
		pLockOn_ = lockOn;
	}

