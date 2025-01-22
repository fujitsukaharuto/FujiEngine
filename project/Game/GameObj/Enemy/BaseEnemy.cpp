#include "BaseEnemy.h"
#include"GameObj/Player/Player.h"
#include"FPSKeeper.h"
#include "Model/Line3dDrawer.h"
///*behavior
#include"Behavior/EnemyRoot.h"
#include"Behavior/EnemyBlowingWeak.h"
#include"Behavior/EnemyBlowingStrong.h"
#include"Behavior/EnemyFall.h"
///* std
#include<algorithm>

///==========================================================
///　static 変数初期化
///==========================================================
float BaseEnemy::InitY_ = 1.5f;
Vector3 BaseEnemy::InitScale_ = { 1.0f,1.0f,1.0f };

BaseEnemy::BaseEnemy() {

}

///========================================================
///  初期化
///========================================================
void BaseEnemy::Initialize() {
	// 基底クラスの初期化
	OriginGameObject::Initialize();

	tags_[static_cast<size_t>(Tag::FALL)] = "FallEnemy";
	tags_[static_cast<size_t>(Tag::BLOWING)] = "BlowingEnemy";

	///spawn
	spawnEasing_.time = 0.0f;
	spawnEasing_.maxTime = 0.8f;
	model_->transform.scale = Vector3::GetZeroVec();

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag(tags_[static_cast<size_t>(Tag::FALL)]);
	collider_->SetWidth(2.0f);
	collider_->SetHeight(2.0f);
	collider_->SetDepth(2.0f);
	collider_->SetParent(model_.get());


	ChangeBehavior(std::make_unique<EnemyFall>(this));/// 追っかけ
}

///========================================================
/// 更新
///========================================================
void BaseEnemy::Update() {

	/// 生成処理
	spawnEasing_.time += FPSKeeper::DeltaTimeRate();
	spawnEasing_.time = min(spawnEasing_.time, spawnEasing_.maxTime);
	model_->transform.scale =
		EaseOutBack(Vector3::GetZeroVec(), BaseEnemy::InitScale_,
			spawnEasing_.time, spawnEasing_.maxTime);

	// 振る舞い更新
	behavior_->Update();

	// collider更新
	collider_->InfoUpdate();

}


///========================================================
/// 描画
///========================================================
void BaseEnemy::Draw(Material* material) {
	OriginGameObject::Draw(material);
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG
}


void BaseEnemy::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
	// fall
	if (collider_->GetTag() == tags_[static_cast<size_t>(Tag::FALL)]) {
		// 弱いキックをくらう
		if (other.tag == pPlayer_->GetTag(static_cast<size_t>(Player::KikPower::WEAK))) {
			collider_->SetTag(tags_[static_cast<size_t>(Tag::BLOWING)]);
			ChangeBehavior(std::make_unique<EnemyBlowingWeak>(this));
			return;
		}

		// 強いキックをくらう
		else if (other.tag == pPlayer_->GetTag(static_cast<size_t>(Player::KikPower::MAXPOWER))) {
			collider_->SetTag(tags_[static_cast<size_t>(Tag::BLOWING)]);
			ChangeBehavior(std::make_unique<EnemyBlowingStrong>(this));
			return;
		}
	}
	//blow
	if (collider_->GetTag() == tags_[static_cast<size_t>(Tag::BLOWING)]) {
		if (other.tag == "UFO") {
			isdeath_ = true;
		}
	}
}

void BaseEnemy::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}


void BaseEnemy::SetParm(const Type& type, const Paramater& paramater) {
	type_ = type;
	paramater_ = paramater;

}


void BaseEnemy::SetPlayer(Player* player) {
	pPlayer_ = player;
}


void BaseEnemy::ChangeBehavior(std::unique_ptr<BaseEnemyBehaivor>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
}

//float BaseEnemy::GetBoundPower()const {
//	// 減衰率を計算 (0～1)
//	float remainingRate = (float(paramater_.deathCountMax) - float((paramater_.deathCountMax - deathCount_))) / float(paramater_.deathCountMax);
//	remainingRate = std::clamp(remainingRate, 0.0f, 1.0f); // 範囲を制限
//
//	// 減衰率に基づくジャンプ速度の計算
//	float baseSpeed = paramater_.boundPower; // 初期ジャンプ力を基準とする
//	float scaledSpeed = baseSpeed * std::pow(remainingRate, paramater_.attenuate); // 減衰を2乗で適用
//	return scaledSpeed;
//}
