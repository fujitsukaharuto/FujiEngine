#include "BaseEnemy.h"
#include"GameObj/Player/Player.h"
#include"FPSKeeper.h"
#include "Model/Line3dDrawer.h"
///*behavior
#include"Behavior/EnemyRoot.h"
#include"Behavior/EnemySpawnFall.h"
#include"Behavior/EnemyBlowingWeak.h"
#include"Behavior/EnemyBlowingStrong.h"
#include"Behavior/EnemyExplotion.h"
#include"Behavior/EnemyFall.h"

#include"Field/Field.h"
///* std
#include<algorithm>

///==========================================================
///　static 変数初期化
///==========================================================
float BaseEnemy::InitY_ = 1.5f;
float BaseEnemy::BoundPosY_ = 42.0f;
Vector3 BaseEnemy::InitScale_ = { 1.0f,1.0f,1.0f };

BaseEnemy::BaseEnemy() {

}

///========================================================
///  初期化
///========================================================
void BaseEnemy::Initialize() {
	// 基底クラスの初期化
	OriginGameObject::Initialize();

	/// teg
	tags_[static_cast<size_t>(Tag::FALL)] = "FallEnemy";
	tags_[static_cast<size_t>(Tag::BLOWINGWEAK)] = "BlowingWeakEnemy";
	tags_[static_cast<size_t>(Tag::BLOWINGSTRONG)] = "BlowingStrongEnemy";

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

	// 初期パラメータセット
	explotionTime_ = paramater_.explotionTime_;
	blowDirection_ = 1.0f;
	ChangeBehavior(std::make_unique<EnemySpawnFall>(this)); /// 追っかけ
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

	WallRefrection(); // 壁反発

	// collider更新
	collider_->InfoUpdate();

}

/// ===================================================
///  Player Jump
/// ===================================================

void BaseEnemy::Bound(float& speed) {
	// 移動
	model_->transform.translate.y += speed * FPSKeeper::NormalDeltaTime();
	SpawnFall(speed, true);

}

///=========================================================
///　落ちる
///==========================================================
void BaseEnemy::SpawnFall(float& speed, const bool& isJump) {
	if (!isJump) {
		// 移動
		model_->transform.translate.y += speed * FPSKeeper::NormalDeltaTime();
	}
	// 加速する
	speed = max(speed - (paramater_.spawnBoundGravity * FPSKeeper::NormalDeltaTime()), -paramater_.spawnBoundGravityMax);

	if (speed <= -paramater_.spawnBoundGravityMax) {// 落ちる

		if (dynamic_cast<EnemySpawnFall*>(behavior_.get())) return;
			// ジャンプ終了
			SetRotationX(0.0f);
			ChangeBehavior(std::make_unique<EnemyFall>(this));
	}
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

	///---------------------------------------------------------------------------------------
	/// 落ちてるとき
	///---------------------------------------------------------------------------------------

	if (collider_->GetTag() == tags_[static_cast<size_t>(Tag::FALL)]) {
		// 弱いキックをくらう
		if (other.tag == pPlayer_->GetTag(static_cast<size_t>(Player::KikPower::WEAK))) {
			collider_->SetTag(tags_[static_cast<size_t>(Tag::BLOWINGWEAK)]);
			ChangeBehavior(std::make_unique<EnemyBlowingWeak>(this));
			return;
		}

		// 強いキックをくらう
		else if (other.tag == pPlayer_->GetTag(static_cast<size_t>(Player::KikPower::MAXPOWER))) {
			collider_->SetTag(tags_[static_cast<size_t>(Tag::BLOWINGSTRONG)]);
			ChangeBehavior(std::make_unique<EnemyBlowingStrong>(this));
			return;
		}

		// 弱いふっとび当たった時
		else if (other.tag == tags_[static_cast<size_t>(Tag::BLOWINGWEAK)]) {
			isdeath_ = true; // 単に消える
			return;
		}

		// 強いふっとび当たった時
		else if (other.tag == tags_[static_cast<size_t>(Tag::BLOWINGSTRONG)]) {
			if (!dynamic_cast<EnemyExplotion*>(behavior_.get())) {
				ChangeBehavior(std::make_unique<EnemyExplotion>(this)); // 爆破
				return;
			}
		}
	}
	///---------------------------------------------------------------------------------------
	/// 吹っ飛んでる時
	///---------------------------------------------------------------------------------------

	if (collider_->GetTag() == tags_[static_cast<size_t>(Tag::BLOWINGWEAK)] ||
		collider_->GetTag() == tags_[static_cast<size_t>(Tag::BLOWINGSTRONG)]) {

		//UFOに当たったら爆破
		if (other.tag == "UFO") {
			if (!dynamic_cast<EnemyExplotion*>(behavior_.get())) {
				ChangeBehavior(std::make_unique<EnemyExplotion>(this));
				return;
			}
		}
		else {
			// 弱いキックを受けてる時
			if (collider_->GetTag() == tags_[static_cast<size_t>(Tag::BLOWINGWEAK)]) {
				if (other.tag == tags_[static_cast<size_t>(Tag::FALL)]) {
					explotionTime_ += paramater_.explotionExtensionTime_;
					return;
				}
			}
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


///=========================================================
///　移動制限
///==========================================================
void BaseEnemy::WallRefrection() {

	// フィールドの中心とスケールを取得
	Vector3 fieldCenter = { 0.0f, 0.0f, 0.0f }; // フィールド中心 
	Vector3 fieldScale = Field::baseScale_;     // フィールドのスケール

	// プレイヤーのスケールを考慮した半径
	float radiusX = fieldScale.x - model_->transform.scale.x;

	// 現在位置が範囲内かチェック
	bool insideX = std::abs(model_->transform.translate.x - fieldCenter.x) <= radiusX;

	///-----------------------------------------------------------
	/// 範囲外なら処理を実行
	///-----------------------------------------------------------
	if (!insideX) {
		// 範囲外の場合の位置補正
		model_->transform.translate.x = std::clamp(
			model_->transform.translate.x,
			fieldCenter.x - radiusX,
			fieldCenter.x + radiusX
		);

		// 範囲外の反発処理
		Vector3 directionToCenter = (fieldCenter - model_->transform.translate).Normalize();
		model_->transform.translate.x += directionToCenter.x * 0.1f; // 軽く押し戻す

		blowDirection_ *= -1.0f;// 向き反転
	}
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
