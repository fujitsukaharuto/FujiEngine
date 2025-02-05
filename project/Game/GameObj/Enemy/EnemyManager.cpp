#include "EnemyManager.h"
#include"GameObj/UFO/Behavior/UFODeath.h"
#include"NormalEnemy.h"
#include"GameObj/UFO/UFO.h"

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
	enemyTypes_[static_cast<size_t>(BaseEnemy::Type::MISSILE)] = "StrongEnemy";

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
void EnemyManager::SpawnEnemy(const BaseEnemy::Type& enemyType, const Vector3& position) {
	if (dynamic_cast<UFODeath*>(pUFO_))return;
	// 通常敵
	if (enemyType == BaseEnemy::Type::NORMAL||
		enemyType == BaseEnemy::Type::LEFTSIDE||
		enemyType == BaseEnemy::Type::RIGHTSIDE) {
		std::unique_ptr<NormalEnemy> enemy;
		enemy = std::make_unique<NormalEnemy>();
		enemy->SetParm(enemyType, paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]);
		enemy->SetOnlyParamater(normalParamater_);
		// 位置初期化とlistに追加
		enemy->Initialize();
		enemy->SetPosition(Vector3(position.x, position.y, position.z));
		enemy->SetPlayer(pPlayer_);// プレイヤーセット
		enemies_.push_back(std::move(enemy));
	}

	// ミサイル
	else if (enemyType == BaseEnemy::Type::MISSILE) {
		std::unique_ptr<MissileEnemy> missle;
		missle = std::make_unique<MissileEnemy>();
		missle->SetParm(BaseEnemy::Type::MISSILE, paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)]);
		missle->SetOnlyParamater(missileParamater_);
		// 位置初期化とlistに追加
		missle->Initialize();
		missle->SetTargetPosX(position.x);
		missle->SetWorldPosition(Vector3(0.0f, UFO::InitY_, 15.0f));
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

	for (auto it = enemies_.begin(); it != enemies_.end(); ) {

		if ((*it)->GetType() == BaseEnemy::Type::NORMAL||
			(*it)->GetType() == BaseEnemy::Type::LEFTSIDE||
			(*it)->GetType() == BaseEnemy::Type::RIGHTSIDE) {
			(*it)->SetParm((*it)->GetType(), paramaters_[static_cast<size_t>(BaseEnemy::Type::NORMAL)]);
		}
		else if ((*it)->GetType() == BaseEnemy::Type::MISSILE) {
			(*it)->SetParm(BaseEnemy::Type::MISSILE, paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)]);
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

	// 普通個別
	globalParameter_->AddItem(groupName_, "leftX", normalParamater_.leftX);
	globalParameter_->AddItem(groupName_, "rightX", normalParamater_.rightX);
	globalParameter_->AddItem(groupName_, "moveValue_", normalParamater_.moveValue_);

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

	// 通常個別
	globalParameter_->SetValue(groupName_, "leftX", normalParamater_.leftX);
	globalParameter_->SetValue(groupName_, "rightX", normalParamater_.rightX);
	globalParameter_->SetValue(groupName_, "moveValue_", normalParamater_.moveValue_);

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

	normalParamater_.leftX = globalParameter_->GetValue<float>(groupName_, "leftX");
	normalParamater_.rightX = globalParameter_->GetValue<float>(groupName_, "rightX");
	normalParamater_.moveValue_ = globalParameter_->GetValue<float>(groupName_, "moveValue_");

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

			ImGui::SeparatorText("個別パラメータ");
			ImGui::DragFloat("生成位置左X", &normalParamater_.leftX, 0.01f);
			ImGui::DragFloat("生成位置右X", &normalParamater_.rightX, 0.01f);
			ImGui::DragFloat("移動量", &normalParamater_.moveValue_, 0.01f);


			ImGuiManager::GetInstance()->UnSetFont();
			ImGui::PopID();

			ImGui::TreePop();
		}

		///---------------------------------------------------------
		/// ストロングな敵
		///----------------------------------------------------------

		if (ImGui::TreeNode(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].c_str())) {
			ImGui::PushID(enemyTypes_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].c_str());

			ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

			ImGui::SeparatorText("出現時");

			ImGui::DragFloat("出現時の落ちる速さ(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].spawnFallSpeed,
				0.01f);

			ImGui::DragFloat("出現時UFOに当たった時のバウンド力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].spawnBoundSpeed,
				0.01f);

			ImGui::DragFloat("バウンド中の重力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].spawnBoundGravity,
				0.01f);

			ImGui::DragFloat("バウンド中の重力(最大)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].spawnBoundGravityMax,
				0.01f);


			ImGui::SeparatorText("出現後");
			ImGui::DragFloat("落ちる速さ(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].fallSpeed,
				0.01f);

			ImGui::DragFloat("弱い初期攻撃力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].weakAttackValue,
				0.01f);

			ImGui::DragFloat("合体した時の攻撃力上昇値",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].plusAttackValue,
				0.01f);


			ImGui::DragFloat("強い攻撃力",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].strongAttackValue,
				0.01f);

			ImGui::DragFloat("弱いキックでの吹っ飛び(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::WEAK)],
				0.01f);

			ImGui::DragFloat("強いキックでの吹っ飛び(毎秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].blowingPower[static_cast<size_t>(BaseEnemy::BlowingPower::MaxPower)],
				0.01f);

			ImGui::DragFloat("爆発する時間(秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].explotionTime_,
				0.01f);

			ImGui::DragFloat("爆発する延長時間(秒)",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].explotionExtensionTime_,
				0.01f);

			ImGui::DragFloat3("スケールアップするサイズ",
				&paramaters_[static_cast<size_t>(BaseEnemy::Type::MISSILE)].scaleUpParm_.x,
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

void EnemyManager::AllDeath() {
	for (auto it = enemies_.begin(); it != enemies_.end(); ) {

		(*it)->SetExplotion();
	
	++it; /// 削除しない場合はイテレータを進める
		
	}

}