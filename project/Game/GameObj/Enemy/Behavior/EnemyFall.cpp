
/// behavior
#include"EnemyFall.h"
#include"EnemyAttack.h"
#include"EnemyBound.h"

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

    speedEase_.maxTime = 0.7f;
    maxSpeed_ = pBaseEnemy_->GetFallSpeed() + 9.0f;
}

EnemyFall::~EnemyFall() {}

void EnemyFall::Update() {
    switch (step_) {
    case Step::FALL:
        ///----------------------------------------------------------------------
        /// 落ちる 
        ///----------------------------------------------------------------------
        SpeedChangeMethod();
        time_ += FPSKeeper::NormalDeltaTime(); // フレーム間の経過時間を加算

        // サイン波で左右に揺れるZ軸回転角を計算
        zRotation_ = amplitude_ * std::sin(frequency_ * time_);

        // 回転を適用
        pBaseEnemy_->SetRotation(Vector3(0, 0, zRotation_));

        // Y軸方向に落下
        pBaseEnemy_->AddPosition(Vector3(0, -fallspeed_ *FPSKeeper::NormalDeltaTime(), 0));

        // 着地判定
        if (pBaseEnemy_->GetTrans().translate.y > BaseEnemy::InitY_) break;
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        step_ = Step::LANDING;

        break;
    case Step::LANDING:
        ///----------------------------------------------------------------------
        /// 着地
        ///----------------------------------------------------------------------
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        pBaseEnemy_->SetWorldPositionY(BaseEnemy::InitY_);
        pBaseEnemy_->DecrementDeathCount();

        step_ = Step::CHANGEATTACK;
        break;
    case Step::CHANGEATTACK:
        ///----------------------------------------------------------------------
        /// 攻撃に切り替え
        ///----------------------------------------------------------------------
            pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyBound>(pBaseEnemy_));
        break;

 
    }
}

void EnemyFall::Debug() {
    // デバッグ用コードがあればここに記述
}

void EnemyFall::SpeedChangeMethod() {
    speedEase_.time += FPSKeeper::NormalDeltaTime();
    fallspeed_ = EaseInCirc(maxSpeed_, pBaseEnemy_->GetFallSpeed(), speedEase_.time, speedEase_.maxTime);

    if (speedEase_.time >= speedEase_.maxTime) {
        speedEase_.time = speedEase_.maxTime;
        fallspeed_ = pBaseEnemy_->GetFallSpeed();
    }
}