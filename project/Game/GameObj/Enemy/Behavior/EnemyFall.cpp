
/// behavior
#include"EnemyFall.h"
#include"EnemyBound.h"
#include"EnemyExplotion.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"
#include <cmath> // sin関数のために必要

// 初期化
EnemyFall::EnemyFall(BaseEnemy* boss)
    : BaseEnemyBehaivor("EnemyFall", boss) {

    rotateAmplitude_ = 0.2f;
    rotateFrequency_ = 2.0f;
    translateAmplitude_ = 0.1f;
    translateFrequency_ = 2.0f;
    time_ = 0.0f;            

    speedEase_.maxTime = 0.7f;
    maxSpeed_ = pBaseEnemy_->GetParamater().fallSpeed + 9.0f;

    step_ = Step::FALL;      // ステップ初期化
}

EnemyFall::~EnemyFall() {}

void EnemyFall::Update() {
    switch (step_) {
    case Step::FALL:
        ///----------------------------------------------------------------------
        /// 落ちる 
        ///----------------------------------------------------------------------
        SpeedChangeMethod();
        time_ += FPSKeeper::DeltaTimeRate(); // フレーム間の経過時間を加算

        // Sinewave
        zRotation_ = rotateAmplitude_ * std::sin(rotateFrequency_ * time_);
        xTranslation_= translateAmplitude_ * std::sin(translateFrequency_ * time_);

        ///* rotate
        pBaseEnemy_->SetRotation(Vector3(0, 0, zRotation_)); // 回転を適用
       
        ///* translate
        pBaseEnemy_->AddPosition(Vector3(xTranslation_, -fallspeed_ *FPSKeeper::DeltaTimeRate(), 0)); // Y軸方向に落下

        ///* explose
        if (pBaseEnemy_->GetTrans().translate.y > BaseEnemy::InitY_-10) break;
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        step_ = Step::LANDING;

        break;
    case Step::LANDING:
        ///----------------------------------------------------------------------
        /// 着地
        ///----------------------------------------------------------------------
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        pBaseEnemy_->SetWorldPositionY(BaseEnemy::InitY_);
        

        step_ = Step::CHANGEATTACK;
        break;
    case Step::CHANGEATTACK:
        ///----------------------------------------------------------------------
        /// 攻撃に切り替え
        ///----------------------------------------------------------------------
            pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyExplotion>(pBaseEnemy_));
        break;

 
    }
}

void EnemyFall::Debug() {
    // デバッグ用コードがあればここに記述
}

void EnemyFall::SpeedChangeMethod() {
    speedEase_.time += FPSKeeper::DeltaTimeRate();
    fallspeed_ = EaseInCirc(maxSpeed_, pBaseEnemy_->GetParamater().fallSpeed, speedEase_.time, speedEase_.maxTime);

    if (speedEase_.time >= speedEase_.maxTime) {
        speedEase_.time = speedEase_.maxTime;
        fallspeed_ = pBaseEnemy_->GetParamater().fallSpeed;
    }
}