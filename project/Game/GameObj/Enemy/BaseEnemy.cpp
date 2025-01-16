#include "BaseEnemy.h"
#include"FPSKeeper.h"
#include "Model/Line3dDrawer.h"
///*behavior
#include"Behavior/EnemyRoot.h"
#include"Behavior/EnemyJump.h"
#include"Behavior/EnemyFall.h"
///* std
#include<algorithm>

///=========================================================
///　static 変数初期化
///==========================================================
float BaseEnemy::InitY_ = 1.5f;
Vector3 BaseEnemy::InitScale_ = {1.0f,1.0f,1.0f};

BaseEnemy::BaseEnemy() {

}

///========================================================
///  初期化
///========================================================
void BaseEnemy::Initialize() {
	// 基底クラスの初期化
	OriginGameObject::Initialize();

	/*model_->transform.translate = spownPos;*/

	///spawn
	spawnEasing_.time = 0.0f;
	spawnEasing_.maxTime = 0.8f;
	deathCount_ = paramater_.deathCountMax;
	model_->transform.scale = Vector3::GetZeroVec();
	
	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag("Enemy");
	collider_->SetWidth(2.0f);
	collider_->SetHeight(2.0f);
	collider_->SetDepth(2.0f);
	collider_->SetParent(model_.get());
	

	jumpCollider_ = std::make_unique<AABBCollider>();
	jumpCollider_->SetTag("EnemyJump");
	jumpCollider_->SetWidth(2.0f);
	jumpCollider_->SetHeight(2.0f);
	jumpCollider_->SetDepth(2.0f);
	jumpCollider_->SetIsCollisonCheck(false);
	jumpCollider_->SetParent(model_.get());
	

	ChangeBehavior(std::make_unique<EnemyFall>(this));/// 追っかけ
}

///========================================================
/// 更新
///========================================================
void BaseEnemy::Update() {

	/// 生成処理
	spawnEasing_.time += FPSKeeper::NormalDeltaTime();
	spawnEasing_.time = min(spawnEasing_.time, spawnEasing_.maxTime);
	model_->transform.scale =
		EaseOutBack(Vector3::GetZeroVec(), BaseEnemy::InitScale_,
		spawnEasing_.time, spawnEasing_.maxTime);

	// 振る舞い更新
	behavior_->Update();

	// collider更新
	collider_->InfoUpdate();
	jumpCollider_->InfoUpdate();
	
	//// 体力がなくなったら死亡
	//if (hp_ <= 0) {
	//	isdeath_=true;
	//	/*Audio::GetInstance()->PlayWave(deathSound_);*/
	//}
}
///========================================================
/// HpBar表示
///========================================================
//void BaseEnemy::DisplayHpBar(const ViewProjection& viewProjection) {
//	// ワールド座標からスクリーン座標に変換
//	Vector3 positionScreen = ScreenTransform(GetWorldPosition(), viewProjection);
//	// Vector2に格納
//	Vector2 positionScreenV2(positionScreen.x-70, positionScreen.y - 90.0f);
//	// Hpバーの座標確定
//	Vector2 hpBarPosition = positionScreenV2;
//	// Hpバーのサイズ
//	hpbar_->SetSize(hpbarSize_);
//	// HPBarスプライト
//	hpbar_->SetPosition(hpBarPosition);
//	// Hpバー更新
//	hpbar_->Update(int(hp_));
//}


///========================================================
/// 描画
///========================================================
void BaseEnemy::Draw(Material*material) {
	OriginGameObject::Draw(material);
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG
}

/// ===================================================
///  Player Jump
/// ===================================================

void BaseEnemy::Jump(float& speed) {
	// 移動
	model_->transform.translate.y += speed;
	Fall(speed, true);

}

///=========================================================
///　落ちる
///==========================================================
void BaseEnemy::Fall(float& speed, const bool& isJump) {
	if (!isJump) {
		// 移動
		model_->transform.translate.y += speed;
	}
	// 加速する
	speed = max(speed - (gravity_ * FPSKeeper::NormalDeltaTime()), -maxFallSpeed_);

	// 着地
	if (model_->transform.translate.y <= BaseEnemy::InitY_) {
		model_->transform.translate.y = BaseEnemy::InitY_;
		speed = 0.0f;

		if (!dynamic_cast<EnemyJump*>(behavior_.get()))return;
		// ジャンプ終了
		SetIsCollision(false);
		SetRotation(Vector3(0, 0, 0));
		ChangeBehavior(std::make_unique<EnemyRoot>(this));
	}
}


//割合によるダメージ
void BaseEnemy::DamageForPar(const float& par) {

	//割合によるインクる面とする値を決める
	float decrementSize = HPMax_ * par;
	// HP減少
	hp_ -= decrementSize;

	////HPが0以下にならないように
	//if (hp_ <= 0) {

	//	hp_ = 0.0f;
	//	isdeath_ = true;
	//	//// 死亡処理
	//	//DeathMethod();
	//	//HP_ = 0.0f;
	//}
}

void BaseEnemy::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	if (other.tag == "WeakKik") {
	/*	if (dynamic_cast<EnemyJump*>(behavior_.get()))return;*/
		jumpPower_ = JumpPower::WEAK;
		ChangeBehavior(std::make_unique<EnemyJump>(this));
	}

	if (other.tag == "UFO") {
		if (!dynamic_cast<EnemyJump*>(behavior_.get()))return;
		isdeath_ = true;
	}

}

void BaseEnemy::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}


void BaseEnemy::SetParm(const Type& type,const Paramater& paramater) {
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


void   BaseEnemy::SetIsCollision(const bool& is) {
	jumpCollider_->SetIsCollisonCheck(is);
}

float BaseEnemy::GetBoundPower()const {
	// 減衰率を計算 (0～1)
	float remainingRate = (float(paramater_.deathCountMax) - float((paramater_.deathCountMax - deathCount_))) / float(paramater_.deathCountMax);
	remainingRate = std::clamp(remainingRate, 0.0f, 1.0f); // 範囲を制限

	// 減衰率に基づくジャンプ速度の計算
	float baseSpeed = paramater_.baseBoundPower; // 初期ジャンプ力を基準とする
	float scaledSpeed = baseSpeed * std::pow(remainingRate, paramater_.attenuate); // 減衰を2乗で適用
	return scaledSpeed;
}


void BaseEnemy::DecrementDeathCount() {
	deathCount_--;
	deathCount_ = max(deathCount_, 0);

}