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
#include"State/EnemyNoneState.h"
#include"State/EnemyPowerUp.h"

#include"GameObj/Field/Field.h"
///* std
#include<algorithm>

///==========================================================
///　static 変数初期化
///==========================================================
float BaseEnemy::InitY_ = 1.0f;
float BaseEnemy::BoundPosY_ = 47.0f;
Vector3 BaseEnemy::BaseScale_ = { 1.0f,1.0f,1.0f };

BaseEnemy::BaseEnemy() {

}

///========================================================
///  初期化
///========================================================
void BaseEnemy::Initialize() {
	// 基底クラスの初期化
	OriginGameObject::Initialize();

	/// teg
	tags_[static_cast<size_t>(Tag::FALL)]           =   "FallEnemy";
	tags_[static_cast<size_t>(Tag::BLOWINGWEAK)]    =   "BlowingWeakEnemy";
	tags_[static_cast<size_t>(Tag::BLOWINGSTRONG)]  =   "BlowingStrongEnemy";

	///spawn
	spawnEasing_.time = 0.0f;
	spawnEasing_.maxTime = 0.8f;
		
	// 初期パラメータセット
	explotionTime_ = paramater_.explotionTime_;
	blowDirection_ = 1.0f;
	sumWeakAttackValue_ = paramater_.weakAttackValue;
	model_->transform.scale = BaseEnemy::BaseScale_;
	powerUpScale_ = BaseEnemy::BaseScale_;

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag(tags_[static_cast<size_t>(Tag::FALL)]);
	collider_->SetOwner(this);
	

	sizeupSE_ = Audio::GetInstance()->SoundLoadWave("sizeup.wav");
	
	ChangeState(std::make_unique<EnemyNoneState>(this)); /// 追っかけ
}

///========================================================
/// 更新
///========================================================
void BaseEnemy::Update() {

	// 振る舞い更新
	state_->Update();
	behavior_->Update();

	WallRefrection(); // 壁反発

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
	speed -= paramater_.spawnBoundGravity * FPSKeeper::NormalDeltaTime();

	if (speed <= -paramater_.spawnBoundGravityMax) {// 落ちる

		if (!dynamic_cast<EnemySpawnFall*>(behavior_.get())) return;
			// ジャンプ終了
			SetRotationX(0.0f);
			parachuteSE_ = Audio::GetInstance()->SoundLoadWave("openParachute.wav");
			Audio::GetInstance()->SoundPlayWave(parachuteSE_, 0.6f);
			ChangeBehavior(std::make_unique<EnemyFall>(this));
	}
}


///========================================================
/// 描画
///========================================================
void BaseEnemy::Draw(Material* material) {
	OriginGameObject::Draw(material);
#ifdef _DEBUG
	/*collider_->DrawCollider();*/
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

		// フィールドに落ちた時
		else if (other.tag == "NormalFieldBlock"|| other.tag == "DaungerousFieldBlock") {
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
			// 弱いキックに当たったら
			if (collider_->GetTag() == tags_[static_cast<size_t>(Tag::BLOWINGWEAK)]) {
				if (other.tag == tags_[static_cast<size_t>(Tag::FALL)]) {
					// パラメータ加算
					explotionTime_ += paramater_.explotionExtensionTime_;  //爆発時間延長
					sumWeakAttackValue_ += paramater_.plusAttackValue;     // 攻撃力アップ
					powerUpScale_ += paramater_.scaleUpParm_;                 // スケールアップ
					
					Audio::GetInstance()->SoundPlayWave(sizeupSE_,0.3f);

					if (!dynamic_cast<EnemyPowerUp*>(state_.get())) {
						ChangeState(std::make_unique<EnemyPowerUp>(this));
					}

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

void BaseEnemy::ChangeState(std::unique_ptr<BaseEnemyState>behavior) {
	//引数で受け取った状態を次の状態としてセット
	state_ = std::move(behavior);
}

///=========================================================
///　移動制限
///==========================================================
void BaseEnemy::WallRefrection() {

	if (dynamic_cast<EnemyFall*>(behavior_.get()))return;

	// フィールドの中心とスケールを取得
	Vector3 fieldCenter = Field::baseTranslate_; // フィールド中心 
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

void BaseEnemy::SetCollisionSize(const Vector3& size) {
	collider_->SetWidth(size.x);
	collider_->SetHeight(size.y);
	collider_->SetDepth(size.z*5.0f);
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
