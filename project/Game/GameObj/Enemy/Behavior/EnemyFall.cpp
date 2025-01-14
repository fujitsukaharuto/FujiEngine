
/// behavior
#include"EnemyFall.h"
#include"EnemyAttack.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"
#include <cmath> // sin関数のために必要

// 初期化
EnemyFall::EnemyFall(BaseEnemy* boss)
    : BaseEnemyBehaivor("EnemyFall", boss) {

    step_ = Step::FALL; // ステップ初期化
    amplitude_ = 0.2f;
    frequency_ = 2.0f;
    time_ = 0.0f;       // 時間カウンタの初期化
}

EnemyFall::~EnemyFall() {}

void EnemyFall::Update() {
    switch (step_) {
    case Step::FALL:
        ///----------------------------------------------------------------------
        /// 落ちる 
        ///----------------------------------------------------------------------
        time_ += FPSKeeper::NormalDeltaTime(); // フレーム間の経過時間を加算

        // サイン波で左右に揺れるZ軸回転角を計算
        zRotation_ = amplitude_ * std::sin(frequency_ * time_);

        // 回転を適用
        pBaseEnemy_->SetRotation(Vector3(0, 0, zRotation_));

        // Y軸方向に落下
        pBaseEnemy_->AddPosition(Vector3(0, -pBaseEnemy_->GetFallSpeed()*FPSKeeper::NormalDeltaTime(), 0));

        // 着地判定
        if (pBaseEnemy_->GetTrans().translate.y > BaseEnemy::InitY_) break;
        pBaseEnemy_->SetRotation(Vector3(0, 0, 0));
        step_ = Step::LANDING;

        break;
    case Step::LANDING:
        ///----------------------------------------------------------------------
        /// 着地
        ///----------------------------------------------------------------------

        step_ = Step::CHANGEATTACK;
        break;
    case Step::CHANGEATTACK:
        ///----------------------------------------------------------------------
        /// 攻撃に切り替え
        ///----------------------------------------------------------------------
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        pBaseEnemy_->SetWorldPositionY(BaseEnemy::InitY_);
        pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyAttack>(pBaseEnemy_));
        break;

 
    }
}

void EnemyFall::Debug() {
    // デバッグ用コードがあればここに記述
}
