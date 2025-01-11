#include "BaseEnemy.h"
#include"FPSKeeper.h"
///*behavior
#include"Behavior/EnemyRoot.h"
#include"Behavior/EnemyJump.h"
///* std
#include<algorithm>

///=========================================================
///　static 変数初期化
///==========================================================
float BaseEnemy::InitY_ = 0.5f;
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
	model_->transform.scale = Vector3::GetZeroVec();

	ChangeBehavior(std::make_unique<EnemyRoot>(this));/// 追っかけ
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

	behavior_->Update();
	
	// 体力がなくなったら死亡
	if (hp_ <= 0) {
		isdeath_=true;
		/*Audio::GetInstance()->PlayWave(deathSound_);*/
	}
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
	speed = max(speed - (gravity_ * FPSKeeper::NormalDeltaTime()), -0.7f);

	// 着地
	if (model_->transform.translate.y <= BaseEnemy::InitY_) {
		model_->transform.translate.y = BaseEnemy::InitY_;
		speed = 0.0f;

		if (!dynamic_cast<EnemyJump*>(behavior_.get()))return;
		// ジャンプ終了
		ChangeBehavior(std::make_unique<EnemyRoot>(this));
	}
}


void BaseEnemy::SetPlayer(Player* player) {
	pPlayer_ = player;
}


void BaseEnemy::ChangeBehavior(std::unique_ptr<BaseEnemyBehaivor>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
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

void BaseEnemy::SetParm(const float& fallSpeed, const float& attackValue, const float& gravity, const std::array<float, 1>& jumpSpeed) {
	fallSpeed_ = fallSpeed;
	attackValue_ = attackValue;
	gravity_ = gravity;
	jumpSpeed_ = jumpSpeed;
}
