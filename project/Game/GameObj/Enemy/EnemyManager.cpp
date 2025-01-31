
#include "EnemyManager.h"
#include"NormalEnemy.h"
#include"GameObj/UFO/UFO.h"

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

}

void  EnemyManager::FSpawn() {

}

///========================================================================================
///  敵の生成
///========================================================================================
void EnemyManager::SpawnEnemy(const std::string& enemyType, const Vector3& position) {


	// 通常敵
	if (enemyType == enemyTypes_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]) {
		std::unique_ptr<NormalEnemy> enemy;
		enemy = std::make_unique<NormalEnemy>();
		enemy->SetParm(BaseEnemy::Type::NORMAL, paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]);
		// 位置初期化とlistに追加
		enemy->Initialize();
		enemy->SetWorldPosition(Vector3(position.x, position.y, position.z));
		enemy->SetPlayer(pPlayer_);// プレイヤーセット
		enemies_.push_back(std::move(enemy));
	}

	// ストロングな敵
	else if (enemyType == enemyTypes_[static_cast<size_t>(BaseEnemy::Type::STRONG)]) {
		std::unique_ptr<MissileEnemy> missle;
		missle = std::make_unique<MissileEnemy>();
		missle->SetParm(BaseEnemy::Type::STRONG, paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)]);
		missle->SetOnlyParamater(missileParamater_);
		// 位置初期化とlistに追加
		missle->Initialize();
		missle->SetTargetPosX(position.x);
		missle->SetWorldPosition(Vector3(0.0f, UFO::InitY_, 10.0f));
		missle->SetPosition();
		missle->SetPlayer(pPlayer_);// プレイヤーセット
		pUFO_->ChangePopBehavior();
		enemies_.push_back(std::move(missle));
	}
}


///========================================================================================
///  更新処理
///========================================================================================
void EnemyManager::Update() {


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
			paramaters_[i].weakAttackValue);

		globalParameter_->AddItem(
			groupName_,
			"plusAttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].plusAttackValue);

		globalParameter_->AddItem(
			groupName_,
			"strongAttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].strongAttackValue);


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

		globalParameter_->AddItem(
			groupName_,
			"scaleUpParm_" + std::to_string(int(i + 1)),
			paramaters_[i].scaleUpParm_);
	}

	// ミサイル個別
	globalParameter_->AddItem(groupName_, "fallWaitTime_", missileParamater_.fallWaitTime_);
	globalParameter_->AddItem(groupName_, "fallPos", missileParamater_.fallPos);
	globalParameter_->AddItem(groupName_, "baseScale", missileParamater_.baseScale);
	globalParameter_->AddItem(groupName_, "expansionScale", missileParamater_.expansionScale);
	globalParameter_->AddItem(groupName_, "nearLimitTime_", missileParamater_.scalingUpTime);
	globalParameter_->AddItem(groupName_, "antipationOffsetPos_", missileParamater_.antipationOffsetPos_);
	globalParameter_->AddItem(groupName_, "firstFallEaseMax", missileParamater_.firstFallEaseMax);
	globalParameter_->AddItem(groupName_, "sideMoveEaseMax", missileParamater_.sideMoveEaseMax);
	globalParameter_->AddItem(groupName_, "scalingEaseMax", missileParamater_.scalingEaseMax);
	globalParameter_->AddItem(groupName_, "antipationEaseMax", missileParamater_.antipationEaseMax);
	globalParameter_->AddItem(groupName_, "scalingEaseMaxAsLimit", missileParamater_.scalingEaseMaxAsLimit);
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
			paramaters_[i].weakAttackValue);

		globalParameter_->SetValue(
			groupName_,
			"plusAttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].plusAttackValue);

		globalParameter_->SetValue(
			groupName_,
			"strongAttackValue" + std::to_string(int(i + 1)),
			paramaters_[i].strongAttackValue);

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

		globalParameter_->SetValue(
			groupName_,
			"scaleUpParm_" + std::to_string(int(i + 1)),
			paramaters_[i].scaleUpParm_);
	}
	// ミサイル個別
	globalParameter_->SetValue(groupName_, "fallWaitTime_", missileParamater_.fallWaitTime_);
	globalParameter_->SetValue(groupName_, "fallPos", missileParamater_.fallPos);
	globalParameter_->SetValue(groupName_, "baseScale", missileParamater_.baseScale);
	globalParameter_->SetValue(groupName_, "expansionScale", missileParamater_.expansionScale);
	globalParameter_->SetValue(groupName_, "nearLimitTime_", missileParamater_.scalingUpTime);
	globalParameter_->SetValue(groupName_, "antipationOffsetPos_", missileParamater_.antipationOffsetPos_);
	globalParameter_->SetValue(groupName_, "firstFallEaseMax", missileParamater_.firstFallEaseMax);
	globalParameter_->SetValue(groupName_, "sideMoveEaseMax", missileParamater_.sideMoveEaseMax);
	globalParameter_->SetValue(groupName_, "scalingEaseMax", missileParamater_.scalingEaseMax);
	globalParameter_->SetValue(groupName_, "antipationEaseMax", missileParamater_.antipationEaseMax);
	globalParameter_->SetValue(groupName_, "scalingEaseMaxAsLimit", missileParamater_.scalingEaseMaxAsLimit);
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

		paramaters_[i].weakAttackValue = globalParameter_->GetValue<float>(
			groupName_,
			"AttackValue" + std::to_string(int(i + 1)));

		paramaters_[i].plusAttackValue = globalParameter_->GetValue<float>(
			groupName_,
			"plusAttackValue" + std::to_string(int(i + 1)));

		paramaters_[i].strongAttackValue = globalParameter_->GetValue<float>(
			groupName_,
			"strongAttackValue" + std::to_string(int(i + 1)));


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

		paramaters_[i].scaleUpParm_ = globalParameter_->GetValue<Vector3>(
			groupName_,
			"scaleUpParm_" + std::to_string(int(i + 1)));

	}
	missileParamater_.fallWaitTime_ = globalParameter_->GetValue<float>(groupName_, "fallWaitTime_");
	missileParamater_.fallPos = globalParameter_->GetValue<float>(groupName_, "fallPos");
	missileParamater_.baseScale = globalParameter_->GetValue<Vector3>(groupName_, "baseScale");
	missileParamater_.expansionScale = globalParameter_->GetValue<Vector3>(groupName_, "expansionScale");
	missileParamater_.scalingUpTime = globalParameter_->GetValue<float>(groupName_, "nearLimitTime_");
	missileParamater_.antipationOffsetPos_ = globalParameter_->GetValue<float>(groupName_, "antipationOffsetPos_");
	missileParamater_.firstFallEaseMax = globalParameter_->GetValue<float>(groupName_, "firstFallEaseMax");
	missileParamater_.sideMoveEaseMax = globalParameter_->GetValue<float>(groupName_, "sideMoveEaseMax");
	missileParamater_.scalingEaseMax = globalParameter_->GetValue<float>(groupName_, "scalingEaseMax");
	missileParamater_.antipationEaseMax = globalParameter_->GetValue<float>(groupName_, "antipationEaseMax");
	missileParamater_.scalingEaseMaxAsLimit = globalParameter_->GetValue<float>(groupName_, "scalingEaseMaxAsLimit");
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

			ImGui::DragFloat("弱い初期攻撃力(%)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].weakAttackValue,
				0.01f, 0, 100);

			ImGui::DragFloat("合体した時の攻撃力上昇値(%)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].plusAttackValue,
				0.01f, 0, 100);

			ImGui::DragFloat("強い攻撃力(%)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].strongAttackValue,
				0.01f, 0, 100);


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

			ImGui::DragFloat3("スケールアップするサイズ",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)].scaleUpParm_.x,
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

			ImGui::DragFloat("弱い初期攻撃力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].weakAttackValue,
				0.01f);

			ImGui::DragFloat("合体した時の攻撃力上昇値",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].plusAttackValue,
				0.01f);


			ImGui::DragFloat("強い攻撃力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].strongAttackValue,
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

			ImGui::DragFloat3("スケールアップするサイズ",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::STRONG)].scaleUpParm_.x,
				0.01f);

			ImGui::SeparatorText("個別パラメータ");
			ImGui::DragFloat("ミサイル落ちるまでの時間", &missileParamater_.fallWaitTime_, 0.01f);
			ImGui::DragFloat("ミサイル初期位置Y", &missileParamater_.fallPos, 0.01f);
			ImGui::DragFloat3("基準スケール", &missileParamater_.baseScale.x, 0.01f);
			ImGui::DragFloat3("膨張スケール", &missileParamater_.expansionScale.x, 0.01f);
			ImGui::DragFloat("スケーリングアップタイム", &missileParamater_.scalingUpTime, 0.01f);
			ImGui::DragFloat("落ちる予備動作の上がる量", &missileParamater_.antipationOffsetPos_, 0.01f);
			ImGui::DragFloat("イージングタイム最初の出現", &missileParamater_.firstFallEaseMax, 0.01f);
			ImGui::DragFloat("イージングタイム横移動", &missileParamater_.sideMoveEaseMax, 0.01f);
			ImGui::DragFloat("イージングタイムスケーリング", &missileParamater_.scalingEaseMax, 0.01f);
			ImGui::DragFloat("イージングタイムスケーリング限界の時", &missileParamater_.scalingEaseMaxAsLimit, 0.01f);
			ImGui::DragFloat("イージングタイム予備動作", &missileParamater_.antipationEaseMax, 0.01f);
			

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

void EnemyManager::SetPlayer(Player* player) {
	pPlayer_ = player;
}
void EnemyManager::SetLockon(LockOn* lockOn) {
	pLockOn_ = lockOn;
}

void EnemyManager::SetUFO(UFO* ufo) {
	pUFO_ = ufo;
}
